#ifndef __WAVE_H
#define __WAVE_H
#include "stm32f1xx_hal.h"
#include "fatfs.h"

//RIFF块
typedef __packed struct
{
	uint32_t ChunkID;		   		//chunk id固定为"RIFF",即0X46464952
	uint32_t ChunkSize ;		  //集合大小=文件总大小-8
	uint32_t Format;	   			//格式;WAVE,即0X45564157
}ChunkRIFF ;
//fmt块
typedef __packed struct
{
	uint32_t ChunkID;		   		//chunk id;这里固定为"fmt ",即0X20746D66
	uint32_t ChunkSize ;		  //子集合大小(不包括ID和Size);这里为:20.
	uint16_t AudioFormat;	  	//音频格式;0X01,表示线性PCM;0X11表示IMA ADPCM
	uint16_t NumOfChannels;		//通道数量;1,表示单声道;2,表示双声道;
	uint32_t SampleRate;			//采样率;0X1F40,表示8Khz
	uint32_t ByteRate;				//字节速率; 
	uint16_t BlockAlign;			//块对齐(字节); 
	uint16_t BitsPerSample;		//单个采样数据大小;4位ADPCM,设置为4
	uint16_t ByteExtraData;		//附加的数据字节;2个; 线性PCM,没有这个参数
}ChunkFMT;	   
//fact块 
typedef __packed struct 
{
	uint32_t ChunkID;		   		//chunk id;这里固定为"fact",即0X74636166;线性PCM没有这个部分
	uint32_t ChunkSize ;		  //子集合大小(不包括ID和Size);这里为:4.
	uint32_t FactSize;	  		//转换成PCM的文件大小; 
}ChunkFACT;
//LIST块 
typedef __packed struct 
{
	uint32_t ChunkID;		   		//chunk id;这里固定为"LIST",即0X74636166;
	uint32_t ChunkSize ;		  //子集合大小(不包括ID和Size);这里为:4. 
}ChunkLIST;

//data块 
typedef __packed struct 
{
	uint32_t ChunkID;		   		//chunk id;这里固定为"data",即0X5453494C
	uint32_t ChunkSize ;		  //子集合大小(不包括ID和Size) 
}ChunkDATA;

//wav头
typedef __packed struct
{ 
	ChunkRIFF riff;						//riff块
	ChunkFMT fmt;  						//fmt块
	ChunkFACT fact;						//fact块 线性PCM,没有这个结构体	 
	ChunkDATA data;						//data块		 
}__WaveHeader; 

//wav 播放控制结构体
typedef __packed struct
{ 
	uint16_t audioformat;			//音频格式;0X01,表示线性PCM;0X11表示IMA ADPCM
	uint16_t nchannels;				//通道数量;1,表示单声道;2,表示双声道; 
	uint16_t blockalign;			//块对齐(字节);  
	uint32_t datasize;				//WAV数据大小 
	uint32_t totsec ;					//整首歌时长,单位:秒
	uint32_t cursec ;					//当前播放时长
	uint32_t bitrate;	   			//比特率(位速)
	uint32_t samplerate;			//采样率 
	uint16_t bps;							//位数,比如16bit,24bit,32bit
	uint32_t datastart;				//数据帧开始的位置(在文件里面的偏移)
}wavctrl; 

#define WAVEFILEBUFSIZE		9216	//1152*2*2*2 为了适应MP3解码的要求
#define WAVETEMPBUFSIZE		9216	//WAVETEMPBUFSIZE>5K&&WAVETEMPBUFSIZE>WAVEFILEBUFSIZE/2(for play wave file)
extern uint8_t WaveFileBuf[WAVEFILEBUFSIZE];
extern uint8_t TempBuf[WAVETEMPBUFSIZE];
extern wavctrl WaveCtrlData;
extern uint8_t CloseFileFlag;									//1:already open file have to close it
extern uint8_t EndFileFlag;										//1:reach the wave file end;2:wait for last transfer;3:finish transfer stop dma
extern __IO uint8_t FillBufFlag;							//0:fill first half buf;1:fill second half buf;0xff do nothing
extern uint32_t DmaBufSize;

uint8_t wave_decode_init(char* fname,wavctrl* wavx);
uint8_t My_I2S2_Init(uint32_t DataFormat,uint32_t AudioFreq);
FRESULT ScanWavefiles (char* path);
uint8_t PlayWaveFile(char* path);
uint32_t FillWaveFileBuf(uint8_t *Buf,uint16_t Size);
uint8_t I2S_WaitFlagStateUntilTimeout(I2S_HandleTypeDef *hi2s, uint32_t Flag, uint32_t Status, uint32_t Timeout);

#endif
