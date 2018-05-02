/*----------------------------------------------------------------------------
* Filename:         point.h
* Description:      Point object used in pong on Keil MCB1700 board
* Author(s):        George Cowan, Alexander Rathke
* Date Modified:    Nov. 13, 2017
*----------------------------------------------------------------------------*/
#ifndef _POINT_H
#define _POINT_H

typedef struct {
    // point defined by x and y co-ordinates
    uint16_t x, y;
} Point;

Point   new_point       (uint16_t x, uint16_t y);
void    shift_point     (Point *p, int16_t shift_x, int16_t shift_y);
bool    point_is_equal  (Point *a, Point *b);

#endif /* _POINT_H */

/******************************************************************************
**                            End Of File
******************************************************************************/

