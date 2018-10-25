/*
 * fm_se.c
 *
 * Created on: 20181012
 * Author: clu
 *
 */

#include "stm32f10x.h"
#include "fm_se.h"
#include "delay.h"

/*SPI Interface*/
#define SET_NSS GPIO_SetBits(GPIOA,GPIO_Pin_4)
#define RESET_NSS	GPIO_ResetBits(GPIOA,GPIO_Pin_4)
#define SET_SCK GPIO_SetBits(GPIOA,GPIO_Pin_5)
#define RESET_SCK	GPIO_ResetBits(GPIOA,GPIO_Pin_5)
#define SET_MOSI GPIO_SetBits(GPIOA,GPIO_Pin_7)
#define RESET_MOSI GPIO_ResetBits(GPIOA,GPIO_Pin_7)
#define GET_MISO GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_6)

#define FRAME_SIZE 128

unsigned char Frame[FRAME_SIZE]; // Frame Buffer

void spi_init()
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	
	//NSS: PA4, output pull up, active low
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	//SCK: PA5, output pull up
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	//MOSI: PA7, output pull up
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	//MISO: PA6, input
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void chip_enable(void)
{
	SET_NSS;
	delay_us(40); //delay after frame
	RESET_NSS;
	delay_us(2); //delay after chip select
}

void chip_disable(void)
{
	delay_us(1); //delay after data
	SET_NSS;
}

void spi_send_bytes(unsigned char data) {
	unsigned char i = 0;

	for (i = 0; i < 8; i++) {
		SET_SCK;
		if((data >> (7 - i))&0x01) SET_MOSI;
		else RESET_MOSI;
		RESET_SCK;
	}
	SET_SCK;
	delay_us(1); //delay after byte
}

unsigned char spi_rcv_bytes(void) {
	unsigned char i;
	unsigned char data = 0;

	for (i = 0; i < 8; i++) {
		RESET_SCK;
		data <<= 1;
		data |= GET_MISO;
		SET_SCK;
	}
	delay_us(1); //delay after byte
	return data;
}

void fm_se_write(unsigned char *pbuf)
{
	unsigned int i,len;
	unsigned char lrc = 0;
	
	chip_enable();
	
	//send command
	spi_send_bytes(0x02);
	delay_us(16); //command delay
	
	spi_send_bytes(*(pbuf+1));
	lrc ^= *(pbuf+1);
	spi_send_bytes(*(pbuf+2));
	lrc ^= *(pbuf+2);
	
	len = *(pbuf+2);
	for(i=0;i<len;i++)
	{
		spi_send_bytes(*(pbuf+i+3));
		lrc ^= *(pbuf+i+3);
	}

	//lrc
	lrc = ~lrc;
	spi_send_bytes(lrc);

	//stop
	chip_disable();
}

void fm_se_read(unsigned char *pbuf)
{	
	unsigned int i, len;
	unsigned char len_low, len_high;
	
	chip_enable();
	
	//get data
	spi_send_bytes(0x03);
	delay_us(16); //command delay

	len_high = spi_rcv_bytes();
	len_low = spi_rcv_bytes();

	if(len_high != 0) len = 0;
	else len = len_low;
	
	for(i = 0; i < len+1; i++) {
		*(pbuf + i) = spi_rcv_bytes();
	}
	
	chip_disable();
}

//if ready return 0
unsigned char fm_se_check_status(void)
{
	unsigned char status = 0;
	chip_enable();
	//send command
	spi_send_bytes(0x05);
	delay_us(16); //command delay
	status = spi_rcv_bytes();
	chip_disable();
	
	return status;
}

//if failed return 0
unsigned char limited_wait()
{
	unsigned int i = 0;
	for(i=0;i<2000;i++)
	{
		if(fm_se_check_status() == 0) return i;
	}
	
	return 0;
}

//init fm se
void fm_se_init(void)
{
	spi_init();
	SET_MOSI;
	delay_us(1);
	SET_SCK;
	delay_us(1);
	SET_NSS;
	delay_ms(10); //delay after power on reset
	
	//choose function
	Frame[0]=0x02;
	Frame[1]=0x00;
	Frame[2]=0x07;
	Frame[3]=0x00;
	Frame[4]=0xa4;
	Frame[5]=0x00;
	Frame[6]=0x00;
	Frame[7]=0x02;
	Frame[8]=0xdf;
	Frame[9]=0x01;
	
	fm_se_write(Frame);
	limited_wait();
	fm_se_read(Frame);
	
	Frame[0]=0x02;
	Frame[1]=0x00;
	Frame[2]=0x08;
	Frame[3]=0x00;
	Frame[4]=0x20;
	Frame[5]=0x00;
	Frame[6]=0x00;
	Frame[7]=0x03;
	Frame[8]=0x11;
	Frame[9]=0x22;
	Frame[10]=0x33;

	fm_se_write(Frame);
	limited_wait();
	fm_se_read(Frame);

}

//return device ID
void fm_se_get_id(unsigned char* p_device_id)
{
	int i = 0;
	chip_enable();
	//send command
	spi_send_bytes(0x9f);
	delay_us(16); //command delay
	
	for(i=0; i<7; i++) {
		*(p_device_id+i) = spi_rcv_bytes();
	}

	chip_disable();
}

//export: p_random
unsigned char fm_se_random_128bits(unsigned char* p_random)
{
	int i = 0;
	
	Frame[0]=0x02;
	Frame[1]=0x00;
	Frame[2]=0x05;
	Frame[3]=0x00;
	Frame[4]=0x84;
	Frame[5]=0x00;
	Frame[6]=0x00;
	Frame[7]=0x10;
	
	fm_se_write(Frame);
	if(limited_wait() == 0) return 0;
	fm_se_read(Frame);
	
	if(!(Frame[16]==0x90&&Frame[17]==0x00)) return 0;
	
	for(i=0; i<16; i++){
		*(p_random+i) = Frame[i+2];
	}
	
	return 1;
}

//generate ecc key
//if succeed return 1
//cost approx. 38ms
unsigned char fm_se_ecc_generate_key(void)
{
	Frame[0]=0x02;
	Frame[1]=0x00;
	Frame[2]=0x0d;
	Frame[3]=0x80;
	Frame[4]=0x30;
	Frame[5]=0x00;
	Frame[6]=0x00;
	Frame[7]=0x08;
	Frame[8]=0xc2;
	Frame[9]=0x02;
	Frame[10]=0x0a;
	Frame[11]=0x98;
	Frame[12]=0xc0;
	Frame[13]=0x02;
	Frame[14]=0x00;
	Frame[15]=0x08;
	
	fm_se_write(Frame);
	if(limited_wait() == 0) return 0;
	fm_se_read(Frame);
	
	if(Frame[0]==0x90&&Frame[1]==0x00) return 1;
	else return 0;
}

//export: p_public_key
//if succeed return 1
unsigned char fm_se_ecc_export_public_key(unsigned char* p_public_key)
{
	int i = 0;
	Frame[0]=0x02;
	Frame[1]=0x00;
	Frame[2]=0x07;
	Frame[3]=0x00;
	Frame[4]=0xa4;
	Frame[5]=0x00;
	Frame[6]=0x00;
	Frame[7]=0x02;
	Frame[8]=0x00;
	Frame[9]=0x08;
	
	fm_se_write(Frame);
	if(limited_wait() == 0) return 0;
	fm_se_read(Frame);
	
	if(!(Frame[0]==0x90&&Frame[1]==0x00)) return 0;
	
	Frame[0]=0x02;
	Frame[1]=0x00;
	Frame[2]=0x05;
	Frame[3]=0x00;
	Frame[4]=0xb0;
	Frame[5]=0x00;
	Frame[6]=0x00;
	Frame[7]=0x40;
	
	fm_se_write(Frame);
	if(limited_wait() == 0) return 0;
	fm_se_read(Frame);
	
	for(i=0; i<64;i++){
		*(p_public_key+i)=Frame[i+2];
	}
	
	return 1;
}

//import: p_sha256; export: p_signature
//if succeed return 1
//cost approx. 21ms
unsigned char fm_se_ecdsa_sign(unsigned char* p_sha256, unsigned char* p_signature)
{
	int i = 0;
	
	Frame[0]=0x02;
	Frame[1]=0x00;
	Frame[2]=0x2d;
	Frame[3]=0x80;
	Frame[4]=0x3e;
	Frame[5]=0x00;
	Frame[6]=0x01;
	Frame[7]=0x28;
	Frame[8]=0xc2;
	Frame[9]=0x02;
	Frame[10]=0x0a;
	Frame[11]=0x98;
	Frame[12]=0xc1;
	Frame[13]=0x82;
	Frame[14]=0x00;
	Frame[15]=0x20;
	
	for(i=0; i<32; i++){
		Frame[i+16] = *(p_sha256+i);
	}
	
	fm_se_write(Frame);
	if(limited_wait() == 0) return 0;
	fm_se_read(Frame);
	
	if(!(Frame[64]==0x90&&Frame[65]==0x00)) return 0;
	
	for(i=0; i<64; i++){
		*(p_signature+i) = Frame[i];
	}
	
	return 1;
}

//import: p_signature, p_sha256
//if valid return 1
//cost approx. 40ms
unsigned char fm_se_ecdsa_verify(unsigned char* p_signature, unsigned char* p_sha256)
{
	int i = 0;
	
	Frame[0]=0x00;
	Frame[1]=0x00;
	Frame[2]=0x6d;
	Frame[3]=0x80;
	Frame[4]=0x3c;
	Frame[5]=0x00;
	Frame[6]=0x01;
	Frame[7]=0x68;
	Frame[8]=0xc0;
	Frame[9]=0x02;
	Frame[10]=0x00;
	Frame[11]=0x08;
	Frame[12]=0xc1;
	Frame[13]=0x82;
	Frame[14]=0x00;
	Frame[15]=0x60;
	
	for(i=0; i<32; i++){
		Frame[i+16] = *(p_sha256+i);
	}
	
	for(i=0; i<64; i++){
		Frame[i+48] = *(p_signature+i);
	}
	
	fm_se_write(Frame);
	if(limited_wait() == 0) return 0;	
	fm_se_read(Frame);
	
	if(Frame[0]==0x90&&Frame[1]==0x00) return 1;
	else return 0;
}

//import: p_private_key
//cost approx. 28ms
unsigned char fm_se_ecc_import_private_key(unsigned char* p_private_key)
{
	int i = 0;
	
	Frame[0]=0x02;
	Frame[1]=0x00;
	Frame[2]=0x2b;
	Frame[3]=0x80;
	Frame[4]=0x3f;
	Frame[5]=0x41;
	Frame[6]=0x01;
	Frame[7]=0x26;
	Frame[8]=0xc2;
	Frame[9]=0x02;
	Frame[10]=0x0a;
	Frame[11]=0x98;
	Frame[12]=0xcb;
	Frame[13]=0x20;
	
	for(i=0; i<32; i++){
		Frame[i+14] = *(p_private_key+i);
	}
		
	fm_se_write(Frame);
	if(limited_wait() == 0) return 0;
	fm_se_read(Frame);
	
	if(Frame[0]==0x90&&Frame[1]==0x00)
		return 1;
	else
		return 0;
}

//import: p_public_key
//cost approx. 18ms
unsigned char fm_se_ecc_import_public_key(unsigned char* p_public_key)
{
	int i = 0;
	
	Frame[0]=0x02;
	Frame[1]=0x00;
	Frame[2]=0x4b;
	Frame[3]=0x80;
	Frame[4]=0x3f;
	Frame[5]=0x41;
	Frame[6]=0x00;
	Frame[7]=0x46;
	Frame[8]=0xc0;
	Frame[9]=0x02;
	Frame[10]=0x00;
	Frame[11]=0x08;
	Frame[12]=0xca;
	Frame[13]=0x40;
	
	for(i=0; i<64; i++){
		Frame[i+14] = *(p_public_key+i);
	}
	
	fm_se_write(Frame);
	if(limited_wait() == 0) return 0;
	fm_se_read(Frame);
	
	if(Frame[0]==0x90&&Frame[1]==0x00)
		return 1;
	else
		return 0;
}

