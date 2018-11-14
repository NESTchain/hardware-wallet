/*
 * usart1.c
 *
 * Created on: 20181108
 * Author: clu
 *
 */
 
#include"usart1.h"

static unsigned char uart1SendingFlag=0; 
static unsigned char uart1ReceivedFlag=0; 

#define UART1_RBUFSIZE 64
#define UART1_TBUFSIZE 64
#define PRIORITY_UART1 6

__IO static unsigned char rBuf[UART1_RBUFSIZE];
__IO static unsigned int curRBufSize=0;
__IO static unsigned int rStartPoint=0;
__IO unsigned char uart1RErrorFlag=0;

__IO static unsigned char tBuf[UART1_TBUFSIZE];
__IO static unsigned int curTBufSize=0;
__IO static unsigned int tStartPoint=0;

static void Uart1ConfigNVIC(void)
{
	NVIC_InitTypeDef NVIC_InitStructure; 
	
	//Enable the USART1 Interrupt
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;	 
	//PRIORITY_UART1
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = PRIORITY_UART1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

void Uart1Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	
	Uart1ConfigNVIC();
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No ;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &USART_InitStructure);
	
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	USART_ITConfig(USART1, USART_IT_TC, ENABLE);
	USART_Cmd(USART1, ENABLE);
}

static void Uart1SendByte(void)
{
	uart1SendingFlag=1;
	USART_SendData(USART1,tBuf[tStartPoint]);
	curTBufSize--;
	tStartPoint++;
	if(tStartPoint==UART1_TBUFSIZE){
		tStartPoint=0;
	}
}

static void AddByteToSendBuf(unsigned char dat)
{
	unsigned int writePoint;
	while(curTBufSize==UART1_TBUFSIZE);
	
	//mask all interrupts lower than PRIORITY_UART1
	__set_BASEPRI(PRIORITY_UART1<<4);
	writePoint=(tStartPoint+curTBufSize)%UART1_TBUFSIZE;
	tBuf[writePoint]=dat;
	curTBufSize++;
	__set_BASEPRI(0);
}

unsigned char Uart1GetByte(void)
{
	unsigned char dat;
	
	__set_BASEPRI(PRIORITY_UART1<<4);
	dat=rBuf[rStartPoint];
	rStartPoint++;
	if(rStartPoint==UART1_RBUFSIZE){
		rStartPoint=0;
	}
	curRBufSize--;
	if(curRBufSize==0)
		uart1ReceivedFlag=0;
	__set_BASEPRI(0);
	return dat;
}

//redirect fputc
int fputc(int ch,FILE *f)
{
	AddByteToSendBuf((unsigned char)ch);
	if(uart1SendingFlag==0){
		Uart1SendByte();
	}
	return (ch);
}

void USART1_IRQHandler(void)
{
	unsigned int rWritePoint;

	if(USART_GetITStatus(USART1, USART_IT_TC) != RESET){
		USART_ClearITPendingBit(USART1, USART_IT_TC);
		if(curTBufSize)
			Uart1SendByte();
		else
			uart1SendingFlag=0;
	}else if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET){
		USART_ClearITPendingBit(USART1, USART_IT_RXNE);
		uart1ReceivedFlag=1;
		if(curRBufSize==UART1_RBUFSIZE){
			uart1RErrorFlag=1;	
		}else{
			rWritePoint=(curRBufSize+rStartPoint)%UART1_RBUFSIZE;
			rBuf[rWritePoint]=USART_ReceiveData(USART1);
			curRBufSize++;
		}
	}
}

//return 1 if received
unsigned char GetUart1ReceivedFlag(void)
{
	return uart1ReceivedFlag;
}

//return 1 if busy sending 
unsigned char GetUart1SendingFlag(void)
{
	return uart1SendingFlag;
}




