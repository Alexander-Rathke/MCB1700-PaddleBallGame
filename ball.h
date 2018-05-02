/*----------------------------------------------------------------------------
* Filename:         ball.h
* Description:      Ball object used in pong on Keil MCB1700 board
* Author(s):        George Cowan, Alexander Rathke
* Date Modified:    Nov. 13, 2017
*----------------------------------------------------------------------------*/
#ifndef _BALL_H
#define _BALL_H

typedef struct {
    /*
    ball defined by a center point,
    radius, color, velocity, and
    bitmap for printing
    */
    Point center;
    uint16_t radius;
    unsigned short color;
    unsigned short *b_map;
    // [x speed, y speed]
    int8_t  velocity[2];
} Ball;

Ball    new_ball            (Point p, unsigned short color);
void    move_ball           (Ball *b, Point p);
void    fill_line           (Ball *b, uint32_t x_pos, uint32_t y_lower, uint32_t y_upper);
void    generate_bitmap     (Ball *b);
void    copy_bitmap         (Ball *from, Ball*to);
void    shift_ball          (Ball *b, int16_t x_shift, int16_t y_shift);
void    set_ball_velocity   (Ball *b, int8_t x_speed, int8_t y_speed);
void    print_bitmap        (Ball *b);
void    draw_ball           (Ball *b);
bool    ball_is_pos_equal   (Ball *a, Ball *b);
Ball    subtract_ball       (Ball *old, Ball *next, unsigned short clear_color);
Ball    deep_copy_ball      (Ball *b);
void    free_bitmap         (Ball *b);
void    erase_ball          (Ball *b, unsigned short clear_color);

#endif /* _BALL_H */

/******************************************************************************
**                            End Of File
******************************************************************************/

