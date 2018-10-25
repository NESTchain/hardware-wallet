/*
 * delay.c
 *
 * Created on: 10/09/2018
 * Author: clu
 *
 */

#include "stm32f10x.h"
#include "delay.h"

void delay_us(unsigned int num)
{
	unsigned int i;
	
	for(i=0; i<num; i++){
		__nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop();
		__nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop();
		__nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop();
		__nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop();
		__nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop();
		__nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop();
		__nop(); __nop(); __nop(); __nop(); __nop();
	}
}

void delay_ms(unsigned int num)
{
	unsigned int i;
	
	for(i=0; i<num; i++){
		delay_us(1000);
	}
}
