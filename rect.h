/*----------------------------------------------------------------------------
* Filename:         rect.h
* Description:      Rectangle object, used for paddle and wall representation
* Author(s):        George Cowan, Alexander Rathke
* Date Modified:    Nov. 13, 2017
*----------------------------------------------------------------------------*/
#ifndef _RECT_H
#define _RECT_H

typedef struct {
    /*
    rectangle defined by bottom left point,
    top right point, and color
    */
    Point b_left, t_right;
    unsigned short color;
} Rect;

Rect    new_rect            (Point b_left, Point t_right, unsigned short color);
void    print_rect          (Rect *r, char * name);
void    print_rect_lite     (Rect *r);
bool    rect_is_pos_equal   (Rect *a, Rect*b);
void    rect_set_points     (Rect *r, Point b_left, Point t_right);
Rect    subtract_rect_y     (Rect *old, Rect *next, unsigned short clear_color);
void    shift_rect          (Rect *r, int16_t shift_x, int16_t shift_y);
void    shift_rect_y        (Rect *r, int16_t shift_y);
void    draw_rect           (Rect *r);

#endif /* _RECT_H */

/******************************************************************************
**                            End Of File
******************************************************************************/
