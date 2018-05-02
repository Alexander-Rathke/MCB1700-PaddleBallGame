/*----------------------------------------------------------------------------
* Filename:         rect.c
* Description:      Rectangle object, used for paddle and wall representation
* Author(s):        George Cowan, Alexander Rathke
* Date Modified:    Nov. 13, 2017
*----------------------------------------------------------------------------*/
#include <lpc17xx.h>
#include <stdio.h>
#include <stdbool.h>
#include "point.h"
#include "GLCD.h"
#include "rect.h"

/*----------------------------------------------------------------------------
 *      Function Definitions
 *---------------------------------------------------------------------------*/

/*******************************************************************************
*   Function Name:      new_rect
*   Author(s):          Alexander Rathke
*   Definition:         rectangle generator
*   Parameters:         bottom left, top right points, rectangle color
*   Returns:            created rectangle
*******************************************************************************/
Rect new_rect(Point b_left, Point t_right, unsigned short color) {
    Rect r;
    r.b_left = b_left;
    r.t_right = t_right;
    r.color = color;
    return r;
}

/*******************************************************************************
*   Function Name:      rect_set_points
*   Author(s):          Alexander Rathke
*   Definition:         assign new bottom left and top right points to rectangle
*   Parameters:         rectangle, new bottom left and top right points
*******************************************************************************/
void rect_set_points(Rect *r, Point b_left, Point t_right) {
    r->b_left = b_left;
    r->t_right = t_right;
}

/*******************************************************************************
*   Function Name:      print_rect
*   Author(s):          Alexander Rathke
*   Definition:         print rectangle point values
*   Parameters:         rectangle, name to print out
*******************************************************************************/
void print_rect(Rect *r, char * name) {
    printf("%s bottom_left.x = %d\r\n", name, r->b_left.x);
    printf("%s bottom_left.y = %d\r\n", name, r->b_left.y);
    printf("%s bottom_right.x = %d\r\n", name, r->t_right.x);
    printf("%s bottom_right.y = %d\r\n", name, r->t_right.y);
}

/*******************************************************************************
*   Function Name:      print_rect_lite
*   Author(s):          Alexander Rathke
*   Definition:         print rect info on one line, no description
*   Parameters:         rectangle
*******************************************************************************/
void print_rect_lite(Rect *r) {
    printf("%d %d %d %d\r\n", r->b_left.x, r->b_left.y, r->t_right.x, r->t_right.y);
}

/*******************************************************************************
*   Function Name:      rect_is_pos_equal
*   Author(s):          Alexander Rathke
*   Definition:         check if points in rect a in same pos as in rect b
*   Parameters:         two rectangles to compare
*   Returns:            bool true if rectangles have same position, false if not
*******************************************************************************/
bool rect_is_pos_equal(Rect *a, Rect*b) {
    return (point_is_equal(&(a->b_left), &(b->b_left)) &&
            point_is_equal(&(a->t_right), &(b->t_right)));
}

/*******************************************************************************
*   Function Name:      subtract_rect_y
*   Author(s):          Alexander Rathke
*   Definition:         returns rectangle containing portion of old rectangle
                        not overlapped by next rectangle
                        assuming both rectangles are parallel in y-axis, equal
                        sized, and not in equal position
                        returned rectangle color with clear_color
*   Parameters:         old rectangle, next rectangle position (in current pos),
                        and a clear color
*   Returns:            rectangle containing area of old not overlapped by next
                        with specified color
*******************************************************************************/
Rect subtract_rect_y(Rect *old, Rect *next, unsigned short clear_color) {
    Rect non_overlap = (*old);
    non_overlap.color = clear_color;

    if ((old->t_right.y > next->b_left.y) && (old->b_left.y < next->b_left.y)){
        // New to right of old, overlapping
        Point top_right = new_point(old->t_right.x, next->b_left.y);
        non_overlap = new_rect(old->b_left, top_right, clear_color);
    }
    else if ((next->t_right.y > old->b_left.y) && (old->b_left.y > next->b_left.y)){
        // New to left of old, overlapping
        Point bottom_left = new_point(old->b_left.x, next->t_right.y);
        non_overlap = new_rect(bottom_left, old->t_right, clear_color);
    }

    return non_overlap;
}

/*******************************************************************************
*   Function Name:      shift_rect
*   Author(s):          Alexander Rathke
*   Definition:         shift rectangle postion (relative shift)
*   Parameters:         rectangle, x-dir shift, y-dir shift
*******************************************************************************/
void shift_rect(Rect *r, int16_t shift_x, int16_t shift_y) {
    shift_point(&(r->b_left), shift_x, shift_y);
    shift_point(&(r->t_right), shift_x, shift_y);
}

/*******************************************************************************
*   Function Name:      shift_rect
*   Author(s):          Alexander Rathke
*   Definition:         shifts rectangle, only in y-direction
*   Parameters:         rectangle, y-dir shift
*******************************************************************************/
void shift_rect_y(Rect *r, int16_t shift_y) {
    shift_point(&(r->b_left), 0, shift_y);
    shift_point(&(r->t_right), 0, shift_y);
}

/*******************************************************************************
*   Function Name:      draw_rect
*   Author(s):          Alexander Rathke
*   Definition:         draws rectangle on LCD
*   Parameters:         rectangle to draw
*******************************************************************************/
void draw_rect(Rect *r) {
    uint16_t i, j;
    GLCD_SetTextColor(r->color);

    for (i = r->b_left.x; i <= r->t_right.x; ++i) {
        for(j = r->b_left.y; j <= r->t_right.y; ++j) {
            GLCD_PutPixel(i,j);
        }
    }
}

/******************************************************************************
**                            End Of File
******************************************************************************/

