/*----------------------------------------------------------------------------
* Filename:         utils.c
* Description:      Utility functions for pong game on Keil MCB1700 board
* Author(s):        George Cowan, Alexander Rathke
* Date Modified:    Nov. 13, 2017
*----------------------------------------------------------------------------*/
#include <lpc17xx.h>
#include <stdbool.h>
#include "utils.h"

/*******************************************************************************
* 	Function Name:		is_bit_on
* 	Definition:			checks if specified bit in number is 1
*   Parameters:      	number (num) and 0 indexed bit position (pos)
*   Returns:            true if bit is 1, False if 0                     
*******************************************************************************/
bool is_bit_on(uint32_t num, uint8_t pos) {
    /*
        Returns 1 if bit at position "pos" in int "num" is 1,
        returns 0 if it is 0.
    
        0 indexed bits.
    */
    return ((num & (1 << pos)) != 0);
}

/******************************************************************************
**                            End Of File
******************************************************************************/
