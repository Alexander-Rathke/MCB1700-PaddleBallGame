/*----------------------------------------------------------------------------
* Filename:         point.c
* Description:      Point object used in pong on Keil MCB1700 board
* Author(s):        George Cowan, Alexander Rathke
* Date Modified:    Nov. 13, 2017
*----------------------------------------------------------------------------*/
#include <lpc17xx.h>
#include <stdbool.h>
#include "point.h"

/*----------------------------------------------------------------------------
 *      Function Definitions
 *---------------------------------------------------------------------------*/

/*******************************************************************************
*   Function Name:      new_point
*   Author(s):          Alexander Rathke
*   Definition:         point generator
*   Parameters:         x and y co-ordinates of point
*   Returns:            point with given x and y co-ordinates
*******************************************************************************/
Point new_point(uint16_t x, uint16_t y) {
    Point p;
    p.x = x;
    p.y = y;
    return p;
}

/*******************************************************************************
*   Function Name:      shift_point
*   Author(s):          Alexander Rathke
*   Definition:         shift point by given x and y relative shift values
*   Parameters:         point, x and y shift values
*******************************************************************************/
void shift_point(Point *p, int16_t shift_x, int16_t shift_y) {
    p->x += shift_x;
    p->y += shift_y;
}

/*******************************************************************************
*   Function Name:      point_is_equal
*   Author(s):          Alexander Rathke
*   Definition:         checks if two points are equal (same co-ordinates)
*   Parameters:         point a, point b
*   Returns:            bool true if equal, false if not
*******************************************************************************/
bool point_is_equal (Point *a, Point *b) {
    return (a->x == b->x &&
            a->y == b->y);
}

/******************************************************************************
**                            End Of File
******************************************************************************/

