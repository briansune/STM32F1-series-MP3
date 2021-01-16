#ifndef __MP3_H
#define __MP3_H
#include "stm32f1xx_hal.h"
#include "fatfs.h"
#define DECODEBUFSIZE	4608	//1152*2�ֽ�*2����
#define MAINBUF_SIZE	1940
extern uint8_t Mp3DecodeBuf[DECODEBUFSIZE];

//ID3V2 ��ǩͷ 
typedef __packed struct 
{
	uint8_t id[3];			//ID
	uint8_t mversion;		//���汾��
	uint8_t sversion;		//�Ӱ汾��
	uint8_t flags;			//��ǩͷ��־
	uint8_t size[4];		//��ǩ��Ϣ��С(��������ǩͷ10�ֽ�).����,��ǩ��С=size+10.
}ID3V2_TagHead;

typedef struct 
{
	uint32_t bitsPerSample;		
	uint32_t OutSamples;		
	uint32_t nChans;
	uint32_t DataStart;
	uint32_t samprate;
}mp3Info;

uint8_t PlayMp3File(char* path);
uint32_t FillMp3Buf(uint8_t *Buf);


#endif
