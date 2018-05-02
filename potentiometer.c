// Potentiometer support

// Written by Bernie Roehl, March 2017

#include <lpc17xx.h>
#include <rtl.h>
#include "potentiometer.h"

void potentiometer_setup(void) {
	LPC_PINCON->PINSEL1 &= ~(3<<18);      // P0.25 is GPIO
	LPC_PINCON->PINSEL1 |=  (1<<18);      // P0.25 is AD0.2
	LPC_SC->PCONP       |=  (1<<12);      // enable power to ADC block
	LPC_ADC->ADCR        =  (1<< 2) |     // select AD0.2 pin
	                        (4<< 8) |     // ADC clock is 25MHz/5
	                        (1<<21);      // enable ADC 
}

uint32_t potentiometer_read(void) {
	LPC_ADC->ADCR |=  (1<<24);             // start A/D conversion     
	while (!(LPC_ADC->ADGDR & (1u << 31))) // wait for conversion
		os_tsk_pass();                     // make sure other tasks can run
	return (LPC_ADC->ADGDR >> 4) & 0xFFF;  // read and return 12-bit result
}
