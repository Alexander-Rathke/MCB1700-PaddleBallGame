/*----------------------------------------------------------------------------
* Filename:         p4_main.c
* Description:      Two player game of pong, for use on a Keil MCB1700 board
* Author(s):        George Cowan, Alexander Rathke
* Date Modified:    Nov. 13, 2017
*----------------------------------------------------------------------------*/
#include <LPC17xx.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <rtl.h>
#include <stdbool.h>
#include "uart.h"
#include "led.h"
#include "GLCD.h"
#include "point.h"
#include "rect.h"
#include "ball.h"
#include "potentiometer.h"
#include "joystick.h"
#include "utils.h"

/*----------------------------------------------------------------------------
 *      Global Variables
 *---------------------------------------------------------------------------*/

// Ball
const unsigned short    BALL_COLOR              =     Yellow;
const uint8_t           BALL_DELAY              =     5;
const uint8_t           DEFAULT_DIRECTION[2]    =     {4,3};
const uint8_t           SPEED_ARRAY[2]          =     {7, 15};
Ball                    main_ball;
uint8_t                 ball_speed;
uint8_t                 speed_index             =     0;

// Game logic
const uint16_t          BORDER_WIDTH            =     10;
const uint16_t          GAME_OVER_DELAY         =     250;
const uint16_t          MAX_ACCEPTABLE_DELAY    =     0x2710;
const uint8_t           MAX_SCORE               =     7;

// Joystick
const uint8_t           JOYSTICK_STEP           =     11;

// Other
const float             PI                      =     3.14159265;

// Paddles
const unsigned short    PADDLE_BOTTOM_COLOR     =     Blue;
const unsigned short    PADDLE_TOP_COLOR        =     Red;
const uint16_t          PADDLE_HEIGHT           =     10;
const uint16_t          PADDLE_OFFSET           =     15;
const uint16_t          PADDLE_WIDTH            =     52;
const uint8_t           TOP_PADDLE_DELAY        =     5;
Rect                    paddle_top;
Rect                    paddle_bottom;

// Score
uint16_t                top_score               =     0;
uint16_t                bottom_score            =     0;
bool                    game_is_over            =     false;

// Mutex
OS_MUT                  lcd_draw_mut;
// Semaphores
OS_SEM                  score_update_ready;
OS_SEM                  signal_top_score;
OS_SEM                  signal_bottom_score;
OS_SEM                  signal_game_over;

/*----------------------------------------------------------------------------
 *      Function Prototypes
 *---------------------------------------------------------------------------*/

void          display_score           ( uint8_t, uint8_t );
void          draw_borders            ( void );
void          display_init            ( void );
void          wait_on_pb              ( void );
void          show_score_page         ( void );
void          init_objects            ( void );
void          reset_ball              ( void );
void          paddle_collision        ( Rect *);
bool          calc_ball_position      ( uint16_t );
void          EINT3_IRQHandler        ( void );
void          redraw_paddles          ( void );

__task  void  tsk_paddle_top          ( void );
__task  void  tsk_paddle_bottom       ( void );
__task  void  tsk_ball                ( void );
__task  void  tsk_top_score           ( void );
__task  void  tsk_bottom_score        ( void );
__task  void  tsk_game_over           ( void );
__task  void  start_tasks             ( void );

int           main                    ( void );

/*----------------------------------------------------------------------------
 *      Function Definitions
 *---------------------------------------------------------------------------*/

/*******************************************************************************
*   Function Name:    display_score
*   Author(s):        Alexander Rathke
*   Definition:       displays each player's score on LEDs, in binary
*   Parameters:       score to show on left and score to show on right of LEDs
*******************************************************************************/
void display_score( uint8_t score_left, uint8_t score_right ) {
    // tracker stores bits to activate for GPIO1 & GPIO2
    // requires no overlapping bits between GPIO1 & GPIO2
    uint32_t tracker = 0,
       	     i;

    // bits to activate for LEDs
    uint8_t left_led[3] = {28, 29, 31};
    uint8_t right_led[3] = {6, 5, 4};

    for (i = 0; i < 3; ++i) {
        if (is_bit_on(score_left, i)) {
            tracker |= (1 << left_led[i]);
        }
    if (is_bit_on(score_right, i)) {
            tracker |= (1 << right_led[i]);
        }
    }

    // activate output
    LPC_GPIO1->FIODIR |= 0xB0000000; //Set bits 28,29,31 of data direction to output
    LPC_GPIO2->FIODIR |= 0x0000007C; //Set bits 2-6 of data direction to output

    // use bit mask to only view portions of tracker we're interested in
    LPC_GPIO1->FIOSET |= (tracker & 0xB0000000);
    LPC_GPIO2->FIOSET |= (tracker & 0x0000007C);
    LPC_GPIO1->FIOCLR |= (~tracker & 0xB0000000);
    LPC_GPIO2->FIOCLR |= (~tracker & 0x0000007C);
}

/*******************************************************************************
*   Function Name:    draw_borders
*   Author(s):        Alexander Rathke
*   Definition:       draw walls on sides of LCD display
*******************************************************************************/
void draw_borders( void ) {
    Rect border_left = new_rect(new_point(0,0), new_point(319,BORDER_WIDTH-1), DarkGrey);
    Rect border_right = new_rect(new_point(0,240-BORDER_WIDTH), new_point(319,239), DarkGrey);

    os_mut_wait(&lcd_draw_mut, 0xFFFF);

    draw_rect(&border_left);
    draw_rect(&border_right);

    os_mut_release(&lcd_draw_mut);
}

/*******************************************************************************
*   Function Name:    display_init
*   Author(s):        Alexander Rathke
*   Definition:       initialize LCD display and clear to black
*******************************************************************************/
void display_init( void ) {
    GLCD_Init();
    GLCD_Clear(Black);
}

/*******************************************************************************
*   Function Name:    wait_on_pb
*   Author(s):        Alexander Rathke
*   Definition:       waits on push button press and release
*******************************************************************************/
void wait_on_pb( void ) {
    uint32_t pos;

    // polls push button
    do {
        pos = (LPC_GPIO2->FIOPIN);
    } while(is_bit_on(pos,10));

    // wait for release of button
    while(!is_bit_on(pos,10)) {
        pos = (LPC_GPIO2->FIOPIN);
    }
}

/*******************************************************************************
*   Function Name:    show_score_page
*   Author(s):        Alexander Rathke
*   Definition:       shows a game over screen with players' scores
*******************************************************************************/
void show_score_page( void ) {
    unsigned char *over = "GAME OVER";
    char red[15],
    blue[15];

    sprintf(red, "RED  - %d", top_score);
    sprintf(blue, "BLUE - %d", bottom_score);

    GLCD_Clear(Black);

    GLCD_SetBackColor(Black);

    // print game over message on LCD
    GLCD_SetTextColor(White);
    GLCD_DisplayString(3,4, 1, over);

    // print red paddle's score on LCD
    GLCD_SetTextColor(Red);
    GLCD_DisplayString(4,4, 1, (unsigned char *) red);

    // print blue paddle's score on LCD
    GLCD_SetTextColor(Blue);
    GLCD_DisplayString(5,4, 1, (unsigned char *) blue);
}

/*******************************************************************************
*   Function Name:    init_objects
*   Author(s):        Alexander Rathke
*   Definition:       defines objects (ball, paddles), clears score display
*******************************************************************************/
void init_objects( void ) {
    uint16_t center_y = ( (BORDER_WIDTH - 1) + (240 - BORDER_WIDTH) ) / 2,
             center_x = 159,
             paddle_left_y = center_y - (PADDLE_WIDTH/2);

    paddle_bottom = new_rect(new_point(PADDLE_OFFSET, paddle_left_y),
                             new_point(PADDLE_OFFSET + PADDLE_HEIGHT, paddle_left_y + PADDLE_WIDTH),
                             PADDLE_BOTTOM_COLOR);
    paddle_top = new_rect(new_point(319 - PADDLE_OFFSET - PADDLE_HEIGHT, paddle_left_y),
                          new_point(319 - PADDLE_OFFSET, paddle_left_y + PADDLE_WIDTH),
                          PADDLE_TOP_COLOR);

    main_ball = new_ball(new_point(center_x, center_y), BALL_COLOR);
    generate_bitmap(&main_ball);

    // set score display to empty (scores 0 at start)
    display_score(top_score,bottom_score);
}

/*******************************************************************************
*   Function Name:    reset_ball
*   Author(s):        Alexander Rathke
*   Definition:       resets ball to center position and default speed
*******************************************************************************/
void reset_ball( void ) {
    uint16_t center_y = ( (BORDER_WIDTH - 1) + (240 - BORDER_WIDTH) ) / 2;
    uint16_t center_x = 159;

    ball_speed = SPEED_ARRAY[0];
    speed_index = 0;

    move_ball(&main_ball, new_point(center_x, center_y));
}

/*******************************************************************************
*   Function Name:    paddle_collision
*   Author(s):        George Cowan
*   Definition:       calculate ball bounce angle with paddle
*   Parameters:       paddle rectangle object
*******************************************************************************/
void paddle_collision( Rect *paddle ) {
    uint8_t min_angle = 15,
            max_angle = 80;
    float bounce_angle;
    int8_t bounce_position = main_ball.center.y - floor((((paddle->b_left.y) + (paddle->t_right.y)) / 2)); //Relative position of ball, where 0 is center of paddle

    //If only a portion of ball is in contact with paddle, bounce with minimum angle.
    if (bounce_position > floor(PADDLE_WIDTH / 2)) {
        bounce_position = floor(PADDLE_WIDTH / 2);
    }
    else if (bounce_position < -1 * floor(PADDLE_WIDTH / 2)) {
        bounce_position = -1 * floor(PADDLE_WIDTH / 2);
    }

    bounce_angle = (min_angle - max_angle) / (PADDLE_WIDTH / 2.0) * abs(bounce_position) + max_angle; //angle of velocity after bounce
    bounce_angle = bounce_angle * PI / 180.0; //Convert to radians

    if (bounce_position <= 0) { //bounce towards left side of screen
        main_ball.velocity[1] = -1 * floor(ball_speed * cos(bounce_angle));
    }
    else { //bounce towards right side of screen
        main_ball.velocity[1] = floor(ball_speed * cos(bounce_angle));
    }

    if (main_ball.velocity[0] >= 0) { //moving upwards
        main_ball.velocity[0] = -1 * ceil(ball_speed * sin(bounce_angle));
    }
    else { //moving downwards
        main_ball.velocity[0] = ceil(ball_speed * sin(bounce_angle));
    }
}

/*******************************************************************************
*   Function Name:    calc_ball_position
*   Author(s):        George Cowan
*   Definition:       calculates next ball position
*   Parameters:       ball move distance
*   Returns:          bool true if goal occurs, false otherwise
*******************************************************************************/
bool calc_ball_position ( uint16_t move_distance ) {
    float scale_factor;
    bool goal_scored = false;
    /*
    Special Move Cases
    1) Hit Right Wall
    2) Hit Left Wall
    3) Current (center - radius) is below top of bottom paddle
       1) if new position is between bounds of paddle & center is above top of paddle -> bounce
       2) if new position is outside bounds of paddle & center is above top of paddle -> allow through
       3) if new position is outside bounds of paddle & center is below top of paddle -> goal is scored
    4) Pass Top of Bottom Paddle (current location is above paddle, next frame will be below paddle)
       1) if new position is between bounds of paddle -> bounce
       2) if new position is outside bounds of paddle & center is above top of paddle -> allow through
       3) if new position is outside bounds of paddle & center is below top of paddle -> goal is scored
    5) Current (center + radius) is above bottom of top paddle
       1) if new position is between bounds of paddle & center is below bottom of paddle -> bounce
       2) if new position is outside bounds of paddle & center is below bottom of paddle -> allow motion
       3) if new position is outside bounds of paddle & center is above bottom of paddle -> goal is scored
    6) Pass Bottom of Top Paddle (current location is below paddle, next frame will be below paddle)
       1) if new position is between bounds of paddle -> bounce
       2) if new position is outside bounds of paddle & center is below bottom of paddle -> allow through
       3) if new position is outside bounds of paddle & center is above bottom of paddle -> goal is scored
    */

    if ((main_ball.center.y + main_ball.radius + main_ball.velocity[1]) > (239 - BORDER_WIDTH)) { //Bounce off right wall
        //(distance moved) / (distance available)
        scale_factor = ((239 - BORDER_WIDTH) - (main_ball.center.y + main_ball.radius)) / (main_ball.velocity[1]);

        //Location of collison
        shift_ball(&main_ball, floor(scale_factor * main_ball.velocity[0]),(239 - BORDER_WIDTH) - (main_ball.center.y + main_ball.radius));

        //Update Velocity Vector
        set_ball_velocity(&main_ball, main_ball.velocity[0], -1*main_ball.velocity[1]);

    }
    else if ((main_ball.center.y - main_ball.radius + main_ball.velocity[1]) < BORDER_WIDTH) { //Bounce off left wall
        //(distance moved) / (distance available)
        scale_factor = -1.0 * ((main_ball.center.y - main_ball.radius) - BORDER_WIDTH) / (main_ball.velocity[1]);

        //Update Ball to location of collision
        shift_ball(&main_ball, floor(scale_factor * main_ball.velocity[0]), main_ball.center.y - main_ball.radius - BORDER_WIDTH );

        //Update Velocity Vector
        set_ball_velocity(&main_ball, main_ball.velocity[0], -1*main_ball.velocity[1]);

    }
    else if ((main_ball.center.x - main_ball.radius) <= paddle_bottom.t_right.x) { //Current (center - radius) is below top of bottom paddle

        // 1) if new position is between bounds of paddle & center is above top of paddle -> bounce
        if (((main_ball.center.x + main_ball.velocity[0]) >= paddle_bottom.t_right.x) &&
            ((main_ball.center.y - main_ball.radius) <= paddle_bottom.t_right.y)    &&
            ((main_ball.center.y + main_ball.radius) >= paddle_bottom.b_left.y))    {

            //shift ball up to collision location
            shift_ball(&main_ball, main_ball.radius - (main_ball.center.x - paddle_bottom.t_right.x) + 1, 0);
            //collide with paddle
            paddle_collision(&paddle_bottom);
        }
        // 2) if new position is outside bounds of paddle & center is above top of paddle -> allow through
        else if ((main_ball.center.x + main_ball.velocity[0]) > paddle_bottom.t_right.x) {

            //move the ball the entire allowable distance
          shift_ball(&main_ball, main_ball.velocity[0], main_ball.velocity[1]);
          }
        // 3) new center is below top of paddle -> goal is scored
        else {
                os_sem_send(&signal_top_score);
                goal_scored = true;
        }
        }
    else if ((main_ball.center.x - main_ball.radius + main_ball.velocity[0]) <= paddle_bottom.t_right.x ) { //Pass Top of Bottom Paddle (current location is above paddle, next frame will be below paddle)

        // 1) if new position is between bounds of paddle -> bounce
        if (((main_ball.center.y - main_ball.radius) <= paddle_bottom.t_right.y) &&
            ((main_ball.center.y + main_ball.radius) >= paddle_bottom.b_left.y)) {

            //scale factor
            scale_factor = -1.0 * (main_ball.center.x - main_ball.radius - paddle_bottom.t_right.x) / main_ball.velocity[0];

            //Update ball to location of collision
            shift_ball(&main_ball, main_ball.center.x - main_ball.radius - paddle_bottom.t_right.x + 1, floor(scale_factor * main_ball.velocity[1]));

            //Update velocity vector
            paddle_collision(&paddle_bottom);
        }
        //2) new position is outside bounds of paddle & center is above top of paddle -> allow through
        else if ((main_ball.center.x - main_ball.radius + main_ball.velocity[0]) > paddle_bottom.t_right.x){
            //move the ball the entire allowable distance
            shift_ball(&main_ball, main_ball.velocity[0], main_ball.velocity[1]);
        }
        //3) new position is outside bounds of paddle & center is below top of paddle -> goal is scored
        else {
            os_sem_send(&signal_top_score);
            goal_scored = true;
        }
    }
    else if ((main_ball.center.x + main_ball.radius) >= paddle_top.b_left.x) { //Current (center + radius) is above bottom of top paddle

        //1) if new position is between bounds of paddle & center is below bottom of paddle -> bounce
        if (((main_ball.center.x + main_ball.velocity[0]) <= paddle_top.b_left.x) &&
           ((main_ball.center.y - main_ball.radius) <= paddle_top.t_right.y)      &&
           ((main_ball.center.y + main_ball.radius) >= paddle_top.b_left.y)) {

            //shift ball down to collision location
            shift_ball(&main_ball, (-1 * ((main_ball.center.x + main_ball.radius) - (paddle_top.b_left.x - main_ball.center.x))) - 1,0);
            //collide with paddle
            paddle_collision(&paddle_top);

        }
        //2) new position is outside bounds of paddle & center is below bottom of paddle -> allow motion
        else if ((main_ball.center.x + main_ball.velocity[0]) < paddle_top.b_left.x) {
            //move the ball the entire allowable distance
            shift_ball(&main_ball, main_ball.velocity[0], main_ball.velocity[1]);
        }
        //3) new position is outside bounds of paddle & center is above bottom of paddle -> goal is scored
        else {
            os_sem_send(&signal_bottom_score);
            goal_scored = true;
        }
    }
    else if ((main_ball.center.x + main_ball.radius + main_ball.velocity[0]) >= paddle_top.b_left.x) { //Pass Bottom of Top Paddle (current location is below paddle, next frame will be above paddle)

        // 1) if new position is between bounds of paddle -> bounce
        if (((main_ball.center.y - main_ball.radius) <= paddle_top.t_right.y) &&
            ((main_ball.center.y + main_ball.radius) >= paddle_top.b_left.y)) {

            //scale factor
            scale_factor = (paddle_top.b_left.x - (main_ball.center.x + main_ball.radius)) / main_ball.velocity[0];

            //Update ball to location of collision
            shift_ball(&main_ball, paddle_top.b_left.x - main_ball.center.x - main_ball.radius - 1, floor(scale_factor * main_ball.velocity[1]));

            //Update velocity vector
            paddle_collision(&paddle_top);

        }
        // 2) new position is outside bounds of paddle & center is below bottom of paddle -> allow through
        else if ((main_ball.center.x + main_ball.radius + main_ball.velocity[0]) < paddle_bottom.b_left.x){
            //move the ball the entire allowable distance
            shift_ball(&main_ball, main_ball.velocity[0], main_ball.velocity[1]);
        }
        // 3) new position is outside bounds of paddle & center is above bottom of paddle -> goal is scored
        else {
            os_sem_send(&signal_bottom_score);
            goal_scored = true;
        }
    }
    else { //No collision occurs -> only move
        shift_ball(&main_ball, main_ball.velocity[0], main_ball.velocity[1]);
    }
    return goal_scored;
}

/*******************************************************************************
*   Function Name:    EINT3_IRQHandler
*   Author(s):        George Cowan
*   Definition:       IRQ handler for push button, cycles ball speed
*******************************************************************************/
void EINT3_IRQHandler ( void ) {
    LPC_GPIOINT->IO2IntClr |= (1 << 10);
    speed_index = (speed_index + 1) % 2; //incrementIndex
    ball_speed = SPEED_ARRAY[speed_index]; //incrementSpeed
}

/*******************************************************************************
*   Function Name:    redraw_paddles
*   Author(s):        Alexander Rathke
*   Definition:       redraws top and bottom paddle
*******************************************************************************/
void redraw_paddles( void ) {
    draw_rect(&paddle_top);
    draw_rect(&paddle_bottom);
}

/*******************************************************************************
*   Function Name:    tsk_paddle_top
*   Author(s):        George Cowan
*   Definition:       task managing top paddle position, controlled by
                      onboard potentiometer
*******************************************************************************/
__task void tsk_paddle_top( void ) {
    uint16_t pot_val,
             bottom_left_y,
             bottom_left_y_old,
             b_left_range = (239 - BORDER_WIDTH - PADDLE_WIDTH) - (BORDER_WIDTH),
             pot_min = 100, //any value less than this maps to max bottom_left_y pixel value
             pot_max = 4000, //value maps to min bottom_left_y pixel value
             pot_range = pot_max - pot_min;
    float step_size = pot_range / b_left_range, //change in potentiometer reading to move paddle 1 pixel
          step_edge;
    uint8_t hysteresis_size = 10;

    // For drawing
    OS_RESULT wait_result;
    Rect paddle_top_old = paddle_top;
    Rect subtractor;

    potentiometer_setup();

    // Initial draw
    draw_rect(&paddle_top);


    //initial poteniometer read - old values for first loop.
    pot_val = potentiometer_read();
    bottom_left_y_old = (uint16_t)ceil((-1.0*b_left_range/pot_range)*pot_val + BORDER_WIDTH + (1.0*pot_max*b_left_range/pot_range));

    while(1) {
        if(!game_is_over) {
            pot_val = potentiometer_read();

            // only allow potentiometer values within max and min range.
            if (pot_val > pot_max) {
                pot_val = pot_max;
            }
            else if (pot_val < pot_min) {
                pot_val = pot_min;
            }

            bottom_left_y = (uint16_t)ceil((-1.0*b_left_range/pot_range)*pot_val + BORDER_WIDTH + (1.0*pot_max*b_left_range/pot_range));


            if (abs(bottom_left_y - bottom_left_y_old) == 1) {
                //Special Case: Paddle was against right border
                if ((bottom_left_y_old == (BORDER_WIDTH + b_left_range)) && (pot_val < (pot_min + hysteresis_size))) {
                    bottom_left_y = bottom_left_y_old;
                }
                //All other cases (except special case: No hysteresis if moving into right border)
                else if (!((bottom_left_y_old == (BORDER_WIDTH + b_left_range - 1)) && (pot_val <= pot_min))) {
                    //edge of old range
                    step_edge = (-1.0*pot_range*bottom_left_y_old/b_left_range) + (BORDER_WIDTH*pot_range/b_left_range) + pot_max;

                    //keep old value if not outside hysteresis zone
                    if (pot_val < ceil(step_edge + hysteresis_size) && pot_val > floor(step_edge - step_size - hysteresis_size)) {
                        bottom_left_y = bottom_left_y_old;
                    }
                }
            }
            bottom_left_y_old = bottom_left_y;

            rect_set_points(&paddle_top, new_point(319-PADDLE_OFFSET-PADDLE_HEIGHT, bottom_left_y), new_point(319-PADDLE_OFFSET, bottom_left_y + PADDLE_WIDTH));

            // Draw updated top paddle
            wait_result = os_mut_wait(&lcd_draw_mut, MAX_ACCEPTABLE_DELAY);
            if (wait_result != OS_R_TMO) {
              if (!rect_is_pos_equal(&paddle_top, &paddle_top_old)) {
                  subtractor = subtract_rect_y(&paddle_top_old, &paddle_top, Black);
                  draw_rect(&paddle_top);
                  draw_rect(&subtractor);
                  paddle_top_old = paddle_top;
              }
              os_mut_release(&lcd_draw_mut);
            }
            os_dly_wait(TOP_PADDLE_DELAY);
        }
        else {
            os_tsk_pass();
        }

    }
}

/*******************************************************************************
*   Function Name:    tsk_paddle_bottom
*   Author(s):        Alexander Rathke
*   Definition:       task managing bottom paddle position, controlled by
                      joystick
*******************************************************************************/
__task void tsk_paddle_bottom( void ) {
    uint32_t pos;
    // maximum position limits
    uint16_t right_lim = (240 - BORDER_WIDTH - JOYSTICK_STEP),
             left_lim = (BORDER_WIDTH - 1 + JOYSTICK_STEP);
    Rect paddle_bottom_old = paddle_bottom,
         subtractor;
    OS_RESULT wait_result;

    // initialize joystick
    joystick_setup();

    // initial draw
    draw_rect(&paddle_bottom);

    while(1) {
        if(!game_is_over) {
            // read joystick, calc values
            pos = joystick_read();

            if (pos == 32 || pos == 33) {
                // move right
                if (paddle_bottom.t_right.y < right_lim) {
                    shift_rect_y(&paddle_bottom, JOYSTICK_STEP);
                }
                else if (paddle_bottom.t_right.y < (240-BORDER_WIDTH) - 1) {
                // if close to limit, shift to max position
                    shift_rect_y(&paddle_bottom, (240-BORDER_WIDTH - 1) - paddle_bottom.t_right.y);
                }
            }
            else if (pos == 8 || pos == 9) {
                // move left
                if (paddle_bottom.b_left.y > left_lim) {
                    shift_rect_y(&paddle_bottom, -1*JOYSTICK_STEP);
                }
                else if (paddle_bottom.b_left.y > BORDER_WIDTH) {
                    shift_rect_y(&paddle_bottom, BORDER_WIDTH - paddle_bottom.b_left.y);
                }
            }

            // Draw updated lower paddle if mut acquired before timeout
            wait_result = os_mut_wait(&lcd_draw_mut, MAX_ACCEPTABLE_DELAY);

            if (wait_result != OS_R_TMO) {
                if (!rect_is_pos_equal(&paddle_bottom, &paddle_bottom_old)) {
                    // clears non-overlapping part of old paddle position
                    subtractor = subtract_rect_y(&paddle_bottom_old, &paddle_bottom, Black);
                    draw_rect(&paddle_bottom);
                    draw_rect(&subtractor);
                    paddle_bottom_old = paddle_bottom;
                }
                os_mut_release(&lcd_draw_mut);
            }
        }
        os_tsk_pass();
    }
}

/*******************************************************************************
*   Function Name:    tsk_ball
*   Author(s):        George Cowan, Alexander Rathke
*   Definition:       task managing ball position
*******************************************************************************/
__task void tsk_ball( void ) {
    Ball main_ball_old = deep_copy_ball(&main_ball),
         subtractor;
    bool goal_scored = false,
         game_was_over = false;
    OS_RESULT wait_result;

    reset_ball();

    // initial draw
    draw_ball(&main_ball);
    set_ball_velocity(&main_ball,DEFAULT_DIRECTION[0],DEFAULT_DIRECTION[1]);

    while(1) {
        if(!game_is_over) {
            if (game_was_over) {
                os_dly_wait(GAME_OVER_DELAY);
                game_was_over = false;
            }

            goal_scored = calc_ball_position(ball_speed);

            if (goal_scored) {
                os_sem_wait(&score_update_ready, 0xFFFF);
            }

            // draw updated ball
            wait_result = os_mut_wait(&lcd_draw_mut, MAX_ACCEPTABLE_DELAY);

            if (wait_result != OS_R_TMO) {
                if (!ball_is_pos_equal(&main_ball_old, &main_ball)) {
                    free_bitmap(&subtractor);
                    copy_bitmap(&main_ball, &main_ball_old);

                    subtractor = subtract_ball(&main_ball_old, &main_ball, Black);

                    // clear non-overlapping portion of old ball position
                    draw_ball(&subtractor);
                    draw_ball(&main_ball);

                    // update old ball location
                    main_ball_old.center = main_ball.center;
                }
                os_mut_release(&lcd_draw_mut);
            }

            // limit ball update speed, otherwise paddle movement significantly
            // slows ball movement speed
            os_dly_wait(BALL_DELAY);
        }
        else {
            game_was_over = true;
            os_tsk_pass();
        }
    }
}

/*******************************************************************************
*   Function Name:    tsk_top_score
*   Author(s):        Alexander Rathke
*   Definition:       updates top paddle score on semaphore signal received
*******************************************************************************/
__task void tsk_top_score( void ) {
    while(1) {
        os_sem_wait(&signal_top_score, 0xFFFF);

        ++top_score;

        draw_borders();
        display_score(top_score, bottom_score);

        if ( top_score == MAX_SCORE) {
            os_sem_send(&signal_game_over);
        }

        reset_ball();
        set_ball_velocity(&main_ball,DEFAULT_DIRECTION[0],DEFAULT_DIRECTION[1]);

        // signal ready to receive another score update
        os_sem_send(&score_update_ready);

        os_tsk_pass();
    }
}

/*******************************************************************************
*   Function Name:    tsk_bottom_score
*   Author(s):        Alexander Rathke
*   Definition:       updates bottom paddle score on semaphore signal received
*******************************************************************************/
__task void tsk_bottom_score( void ) {
    while(1) {
        os_sem_wait(&signal_bottom_score, 0xFFFF);

        ++bottom_score;

        draw_borders();
        display_score(top_score, bottom_score);

        if ( bottom_score == MAX_SCORE ) {
            os_sem_send(&signal_game_over);
        }

        reset_ball();
        set_ball_velocity(&main_ball, -1 * DEFAULT_DIRECTION[0],DEFAULT_DIRECTION[1]);

        // signal ready to receive another score update
        os_sem_send(&score_update_ready);

        os_tsk_pass();
    }
}

/*******************************************************************************
*   Function Name:    tsk_game_over
*   Author(s):        Alexander Rathke
*   Definition:       responds to game over signal, shows score page until
                      push button pressed to start new game
*******************************************************************************/
__task void tsk_game_over( void ) {
    uint8_t i;

    while(1) {
        os_sem_wait(&signal_game_over, 0xFFFF);
        game_is_over = true;

        erase_ball(&main_ball, Black);

        // flash LEDs
        for(i = 0; i < 6; ++i) {
            display_score(0, 0);
            os_dly_wait(16);
            display_score(top_score, bottom_score);
            os_dly_wait(16);
        }

        // prevent push button from registering as ball speed change
        __disable_irq();

        show_score_page();
        // waits on push button press and release to start a new game
        wait_on_pb();

        __enable_irq();

        // reset display for new game
        GLCD_Clear(Black);
        draw_borders();
        reset_ball();
        redraw_paddles();

        // reset score
        top_score = 0;
        bottom_score = 0;
        display_score(top_score, bottom_score);

        game_is_over = false;
        os_tsk_pass();
    }
}

/*******************************************************************************
*   Function Name:    start_tasks
*   Author(s):        George Cowan, Alexander Rathke
*   Definition:       init mutex and semaphores, draw borders, start tasks
*******************************************************************************/
__task void start_tasks( void ) {
    // control LCD screen access, maintain consistent color
    os_mut_init(&lcd_draw_mut);

    os_sem_init(&signal_top_score, 0);
    os_sem_init(&signal_bottom_score, 0);
    os_sem_init(&signal_game_over, 0);
    os_sem_init(&score_update_ready, 0);

    // draw walls of display
    draw_borders();

    // object tasks
    os_tsk_create(tsk_paddle_top, 1);
    os_tsk_create(tsk_paddle_bottom, 1);
    os_tsk_create(tsk_ball, 1);

    // scoring tasks
    os_tsk_create(tsk_bottom_score, 1);
    os_tsk_create(tsk_top_score, 1);
    os_tsk_create(tsk_game_over, 1);

    os_tsk_delete_self();
}

/*******************************************************************************
*   Function Name:    main
*   Author(s):        George Cowan, Alexander Rathke
*   Definition:       init system, config push button interrupt, run program
*******************************************************************************/
int main( void ) {
    SystemInit();
    init_objects();
    display_init();

    // push button interrupt config
    LPC_PINCON->PINSEL4 &= ~( 3 << 20 );
    LPC_GPIO2->FIODIR &= ~( 1 << 10 );
    LPC_GPIOINT->IO2IntEnF |= (1 << 10 );
    NVIC_EnableIRQ( EINT3_IRQn );

    os_sys_init(start_tasks);

    while(1);
}

/******************************************************************************
**                            End Of File
******************************************************************************/

