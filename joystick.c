// Joystick and pushbutton support

// Written by Bernie Roehl, March 2017

#include <lpc17xx.h>
#include "joystick.h"

void joystick_setup(void) {
	LPC_PINCON->PINSEL3 &= ~((3<< 8)|(3<<14)|(3<<16)|(3<<18)|(3<<20));
	LPC_GPIO1->FIODIR &= ~((1<<20)|(1<<23)|(1<<24)|(1<<25)|(1<<26)); 
}

uint32_t joystick_read(void) {
	return 0x79 & ~(LPC_GPIO1->FIOPIN >> 20);
}

void pushbutton_setup(void) {
	LPC_PINCON->PINSEL4 &= ~(3<<20);           // P2.10 is GPIO (INT0)
	LPC_GPIO2->FIODIR   &= ~(1<<10);           // P2.10 is input
}

uint32_t pushbutton_read(void) {  // returns non-zero if button is down
	return (LPC_GPIO2->FIOPIN >> 10) & 0x01;
}
