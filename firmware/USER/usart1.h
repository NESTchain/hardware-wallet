/*
 * usart1.h
 *
 * Created on: 20181108
 * Author: clu
 *
 */

#ifndef __USART1_H__
#define	__USART1_H__

#include"stm32f10x.h"
#include"stdio.h"

//if you want to send something, just call printf
//because we redirect fputc in studio
//select target -> use microLib

void Uart1Init(void);
unsigned char GetUart1ReceivedFlag(void);
unsigned char Uart1GetByte(void);

#endif
