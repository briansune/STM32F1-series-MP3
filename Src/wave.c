#include "wave.h"
#include "string.h"
//#include "wm8978.h"
#include "i2s.h"
#include "mp3.h"


__align(8) uint8_t WaveFileBuf[WAVEFILEBUFSIZE];
__align(8) uint8_t TempBuf[WAVETEMPBUFSIZE];
wavctrl WaveCtrlData;
FIL WavFile;
uint8_t CloseFileFlag;									//1:already open file have to close it
uint8_t EndFileFlag;										//1:reach the wave file end;2:wait for last transfer;3:finish transfer stop dma
__IO uint8_t FillBufFlag;								//0:fill first half buf;1:fill second half buf;0xff do nothing


//打开音乐文件，获得WAVE文件的参数
uint8_t wave_decode_init(char* fname,wavctrl* wavx)
{
	uint32_t br=0;
	uint8_t res=0;
	ChunkRIFF *riff;
	ChunkFMT *fmt;
	ChunkFACT *fact;
	ChunkDATA *data;
	res=f_open(&WavFile,(TCHAR*)fname,FA_READ);												//打开文件
	if(res==FR_OK)
	{
		CloseFileFlag=1;
		f_read(&WavFile,TempBuf,WAVEFILEBUFSIZE/2,&br);									//读取WAVEFILEBUFSIZE/2字节数据
		riff=(ChunkRIFF *)TempBuf;																			//获取RIFF块
		if(riff->Format==0X45564157)																		//是WAV文件
		{
			fmt=(ChunkFMT *)(TempBuf+12);																	//获取FMT块
				if(fmt->AudioFormat==1||fmt->AudioFormat==3)								//线性PCM或32位WAVE=3
				{
					fact=(ChunkFACT *)(TempBuf+12+8+fmt->ChunkSize);					//读取FACT块
					if(fact->ChunkID==0X74636166||fact->ChunkID==0X5453494C)
						wavx->datastart=12+8+fmt->ChunkSize+8+fact->ChunkSize;	//具有fact/LIST块的时候(未测试)
					else 
						wavx->datastart=12+8+fmt->ChunkSize;  
					data=(ChunkDATA *)(TempBuf+wavx->datastart);					
					if(data->ChunkID==0X61746164)															//读取DATA块成功
					{
						wavx->audioformat=fmt->AudioFormat;											//音频格式
						wavx->nchannels=fmt->NumOfChannels;											//通道数
						wavx->samplerate=fmt->SampleRate;												//采样率
						wavx->bitrate=fmt->ByteRate*8;													//位速率=通道数×每秒数据位数×每样本的数据位数
						wavx->blockalign=fmt->BlockAlign;												//块对齐=通道数×每样本的数据位值／8
						wavx->bps=fmt->BitsPerSample;														//位数,8/16/24/32位
						wavx->datasize=data->ChunkSize;													//音频数据块大小
						wavx->datastart=wavx->datastart+8;											//数据流开始的地方. 
						printf("wavx->audioformat:%d\r\n",wavx->audioformat);
						printf("wavx->nchannels:%d\r\n",wavx->nchannels);
						printf("wavx->samplerate:%d\r\n",wavx->samplerate);
						printf("wavx->bitrate:%d\r\n",wavx->bitrate);
						printf("wavx->blockalign:%d\r\n",wavx->blockalign);
						printf("wavx->bps:%d\r\n",wavx->bps);
						printf("wavx->datasize:%d\r\n",wavx->datasize);
						printf("wavx->datastart:%d\r\n",wavx->datastart);  
					}
					else 
						res=4;																									//data区域未找到.
				}
				else
					res=3;																										//非线性PCM，不支持
		}
		else 
			res=2;																												//非wav文件	
	}
	else 
		res=1;																													//打开文件错误
	return res;
}

FRESULT ScanWavefiles (char* path)
{
    FRESULT res;
    FILINFO fno;
    DIR dir;
		uint8_t i;
		char PathBuf[50];
		uint32_t PlayCount=0;
		while(1)
	{
    res = f_opendir(&dir, path);                       							/* Open the directory */
		//printf("%d\r\n", res);
    if (res == FR_OK) 
		{
        for (;;) 
				{
            res = f_readdir(&dir, &fno);                   					/* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0)  								/* Break on error or end of dir */
							break;
						//printf("%s\r\n", fno.fname);
            if ((fno.fname[0] == '.')||((fno.fattrib & AM_DIR)!=0))   
							continue;																							/* Ignore dot entry and directory*/ 
						for(i=0;i<13;i++)
						{
							if(fno.fname[i]=='.')
								break;
						}
						if(i>8)
							continue;
            if (((fno.fname[i+1] == 'w')||(fno.fname[i+1] == 'W'))
							&&((fno.fname[i+2] == 'a')||(fno.fname[i+2] == 'A'))
						&&((fno.fname[i+3] == 'v')||(fno.fname[i+3] == 'V'))) 
						{
                strcpy(PathBuf,path);
								strcat(PathBuf,"/");
								strcat(PathBuf,fno.fname);
								printf("%s\r\n", PathBuf);
								PlayCount++;
								printf("PlayCount=%d\r\n", PlayCount);
								PlayWaveFile(PathBuf);		
						}
						else
						if (((fno.fname[i+1] == 'm')||(fno.fname[i+1] == 'M'))
							&&((fno.fname[i+2] == 'p')||(fno.fname[i+2] == 'P'))
						&&(fno.fname[i+3] == '3')) 
						{
                strcpy(PathBuf,path);
								strcat(PathBuf,"/");
								strcat(PathBuf,fno.fname);
								printf("%s\r\n", PathBuf);
								PlayCount++;
								printf("PlayCount=%d\r\n", PlayCount);
								PlayMp3File(PathBuf);		
						}   
        }
        f_closedir(&dir);
    }
	}
    return res;
}

uint8_t PlayWaveFile(char* path)
{
	uint8_t res;
	CloseFileFlag=0;
	EndFileFlag=0;
	FillBufFlag=0xFF;
	res=wave_decode_init(path,&WaveCtrlData);
	printf("%d\r\n",res);
	//res=WM8978_Init();
	//printf("%d\r\n",res);
	res=My_I2S2_Init(WaveCtrlData.bps,WaveCtrlData.samplerate);
	printf("%d\r\n",res);
	f_lseek(&WavFile, WaveCtrlData.datastart);
	FillWaveFileBuf(WaveFileBuf,WAVEFILEBUFSIZE/2);
	FillWaveFileBuf(&WaveFileBuf[WAVEFILEBUFSIZE/2],WAVEFILEBUFSIZE/2);
	if(WaveCtrlData.bps==8||WaveCtrlData.bps==16)
		HAL_I2S_Transmit_DMA(&hi2s2,(uint16_t *)WaveFileBuf,WAVEFILEBUFSIZE/2);//size是样本数，不是字节数，DMA样本为16位
	else
	if(WaveCtrlData.bps==24||WaveCtrlData.bps==32)
		HAL_I2S_Transmit_DMA(&hi2s2,(uint16_t *)WaveFileBuf,WAVEFILEBUFSIZE/4);//size是样本数，不是字节数，DMA样本为32位
	while(1)
	{
		if(EndFileFlag==0)
		{
			if(FillBufFlag==0)
			{
				FillWaveFileBuf(WaveFileBuf,WAVEFILEBUFSIZE/2);
				FillBufFlag=0xFF;
			}
			else
			if(FillBufFlag==1)
			{
				FillWaveFileBuf(&WaveFileBuf[WAVEFILEBUFSIZE/2],WAVEFILEBUFSIZE/2);
				FillBufFlag=0xFF;
			}
		}
		else
		if(EndFileFlag==3)
		{
			HAL_I2S_DMAStop(&hi2s2);
			//printf("sr=%d\r\n",hi2s2.Instance->SR);
			//printf("cr2=%d\r\n",hi2s2.Instance->CR2);
			//printf("cfgr=%d\r\n",hi2s2.Instance->I2SCFGR);
			__HAL_I2S_ENABLE(&hi2s2);
			res=I2S_WaitFlagStateUntilTimeout(&hi2s2, I2S_FLAG_BSY, SET, 20);
			//printf("res=%d\r\n",res);
			__HAL_I2S_DISABLE(&hi2s2);
			//printf("sr=%d\r\n",hi2s2.Instance->SR);
			break;
		}
	}
	if(CloseFileFlag)
		f_close(&WavFile);
	return 0;
}

uint8_t My_I2S2_Init(uint32_t DataFormat,uint32_t AudioFreq)
{

  hi2s2.Instance = SPI2;
  hi2s2.Init.Mode = I2S_MODE_MASTER_TX;
  hi2s2.Init.Standard = I2S_STANDARD_PHILIPS;
  hi2s2.Init.MCLKOutput = I2S_MCLKOUTPUT_ENABLE;
  hi2s2.Init.CPOL = I2S_CPOL_LOW;
	if(DataFormat==8||DataFormat==16)
	{
		//下面两种模式都可以，在STM32处程序相同，I2S_DATAFORMAT_16B_EXTENDED为32位帧结构
		//(低16位由系统生成)，I2S_DATAFORMAT_16B为16位帧结构
		//hi2s2.Init.DataFormat=I2S_DATAFORMAT_16B_EXTENDED;
		hi2s2.Init.DataFormat=I2S_DATAFORMAT_16B;
		//WM8978_I2S_Cfg(2,0);
	}
	else
	if(DataFormat==24)
	{
		//I2S_DATAFORMAT_24B也是以32位帧的形式进行发送，也可以设置为I2S_DATAFORMAT_32B
		//区别在于I2S_DATAFORMAT_24B时，32位数据中的最低字节系统忽略，会自动清0，可以不用设置
		//I2S_DATAFORMAT_32B时，32位数据中的最低字节要自己清0
		hi2s2.Init.DataFormat=I2S_DATAFORMAT_24B;
		//WM8978_I2S_Cfg(2,3);
	}
	else
	if(DataFormat==32)
	{
		hi2s2.Init.DataFormat=I2S_DATAFORMAT_32B;
		//WM8978_I2S_Cfg(2,3);
	}
	else
		return 1;//not support
	if(AudioFreq==I2S_AUDIOFREQ_8K||AudioFreq==I2S_AUDIOFREQ_11K||
			AudioFreq==I2S_AUDIOFREQ_16K||AudioFreq==I2S_AUDIOFREQ_22K||
				AudioFreq==I2S_AUDIOFREQ_32K||AudioFreq==I2S_AUDIOFREQ_44K||
					AudioFreq==I2S_AUDIOFREQ_48K||AudioFreq==I2S_AUDIOFREQ_96K)
		hi2s2.Init.AudioFreq = AudioFreq;
	else
		return 2;
  if (HAL_I2S_Init(&hi2s2) != HAL_OK)
  {
    Error_Handler();
  }
	return 0;
}

uint32_t FillWaveFileBuf(uint8_t *Buf,uint16_t Size)
{
	uint32_t NeedReadSize=0;
	uint32_t ReadSize;
	uint32_t i;
	uint8_t *p;
	float *f;
	int sound;
	//上一次已经读完了，直接返回
	if(EndFileFlag==1)
		return 0;
	
	//stm32是little endian,Wave文件低地址位上放的是音频数据的低位
	//先放左声道，再放右声道，I2S传输时，先传数据的高位，再传低位以16位为单位传输
	//双声道
	if(WaveCtrlData.nchannels==2)
	{
		if(WaveCtrlData.bps==16)																	//16bit数据直接读取
			f_read(&WavFile,Buf,Size,(UINT*)&ReadSize);
		else
		if(WaveCtrlData.bps==24)																	//24bit音频,需要调整读取数据与DMA缓存中的顺序
		{
			NeedReadSize=(Size/4)*3;																//此次要读取的字节数
			f_read(&WavFile,TempBuf,NeedReadSize,(UINT*)&ReadSize);	//读取数据
			p=TempBuf;
			ReadSize=(ReadSize/3)*4;																//填充后的大小.
			//printf("%d\r\n",ReadSize);
			for(i=0;i<ReadSize;)
			{
				Buf[i]=p[1];
				Buf[i+1]=p[2];
				Buf[i+2]=0;
				Buf[i+3]=p[0];
				i+=4;
				p+=3;
			} 
		}
		else
		if(WaveCtrlData.bps==8)																		//8bit音频数据要转换为16位模式进行播放
		{
			NeedReadSize=Size/2;																		//此次要读取的字节数
			f_read(&WavFile,TempBuf,NeedReadSize,(UINT*)&ReadSize);	//读取数据
			p=TempBuf;
			ReadSize=ReadSize*2;																		//填充后的大小.
			for(i=0;i<ReadSize;)
			{
				Buf[i]=0;
				Buf[i+1]=*p+0x80; 
				p++;
				i=i+2;
			} 	
		}
		else																											//32bit WAVE 浮点数-1至1表示声音
		{
			f_read(&WavFile,TempBuf,Size,(UINT*)&ReadSize);					//读取数据
			f=(float*)TempBuf;
			for(i=0;i<ReadSize;)
			{
				//printf("f=%f\r\n",*f);
				sound=0x7FFFFFFF*(*f);
				Buf[i]=(uint8_t)(sound>>16);
				Buf[i+1]=(uint8_t)(sound>>24);
				Buf[i+2]=(uint8_t)(sound);
				Buf[i+3]=(uint8_t)(sound>>8); 
				f++;
				i=i+4;
			} 						
		}
	}
	//单声道，调整为双声道数据进行播放
	else
	{
		if(WaveCtrlData.bps==16)
		{
			NeedReadSize=Size/2;																		//此次要读取的字节数
			f_read(&WavFile,TempBuf,NeedReadSize,(UINT*)&ReadSize);	//读取数据
			p=TempBuf;
			ReadSize=ReadSize*2;																		//填充后的大小.
			for(i=0;i<ReadSize;)
			{
				Buf[i]=p[0];
				Buf[i+1]=p[1]; 
				Buf[i+2]=p[0];
				Buf[i+3]=p[1];
				i+=4;
				p=p+2;
			}
		}
		else
		if(WaveCtrlData.bps==24)																	//24bit音频
		{
			NeedReadSize=(Size/8)*3;																//此次要读取的字节数
			f_read(&WavFile,TempBuf,NeedReadSize,(UINT*)&ReadSize);	//读取数据
			p=TempBuf;
			ReadSize=(ReadSize/3)*8;																//填充后的大小.
			for(i=0;i<ReadSize;)
			{
				Buf[i]=p[1];
				Buf[i+1]=p[2]; 
				Buf[i+2]=0;
				Buf[i+3]=p[0];
				Buf[i+4]=p[1];
				Buf[i+5]=p[2];
				Buf[i+6]=0;
				Buf[i+7]=p[0];
				p+=3;
				i+=8;
			} 
			
		}
		else
		if(WaveCtrlData.bps==8)																		//8bit音频
		{
			NeedReadSize=Size/4;																		//此次要读取的字节数
			f_read(&WavFile,TempBuf,NeedReadSize,(UINT*)&ReadSize);	//读取数据
			p=TempBuf;
			ReadSize=ReadSize*4;																		//填充后的大小.
			for(i=0;i<ReadSize;)
			{
				Buf[i]=0;
				Buf[i+1]=*p+0x80; 
				Buf[i+2]=0;
				Buf[i+3]=*p+0x80;
				i+=4;
				p++;
			} 
		}
		else																											//32bit
		{
			NeedReadSize=Size/2;																		//此次要读取的字节数
			f_read(&WavFile,TempBuf,NeedReadSize,(UINT*)&ReadSize);	//读取数据
			f=(float*)TempBuf;
			ReadSize=ReadSize*2;																		//填充后的大小.
			for(i=0;i<ReadSize;)
			{
				sound=0x7FFFFFFF*(*f);
				Buf[i+4]=Buf[i]=(uint8_t)(sound>>16);
				Buf[i+5]=Buf[i+1]=(uint8_t)(sound>>24);
				Buf[i+6]=Buf[i+2]=(uint8_t)(sound);
				Buf[i+7]=Buf[i+3]=(uint8_t)(sound>>8); 				
				f++;
				i=i+8;
			} 
		}
	}
	if(ReadSize<Size)																						//不够数据了,补充0
	{
		EndFileFlag=1;
		for(i=ReadSize;i<Size-ReadSize;i++)
			Buf[i]=0;
	}
	//printf("ReadSize=%d\r\n",ReadSize);
	return ReadSize;
}

void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
	/* Prevent unused argument(s) compilation warning */
  UNUSED(hi2s);
  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_I2S_TxHalfCpltCallback could be implemented in the user file
   */
	if(EndFileFlag==0)
	{
		FillBufFlag=0;
	}
	else
	if(EndFileFlag==1)
	{
		memset(WaveFileBuf,0,WAVEFILEBUFSIZE/2);
		EndFileFlag=2;
	}	
	else
	if(EndFileFlag==2)
		EndFileFlag=3;

}

void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)
{
	/* Prevent unused argument(s) compilation warning */
  UNUSED(hi2s);
  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_I2S_TxCpltCallback could be implemented in the user file
   */
	if(EndFileFlag==0)
	{
		FillBufFlag=1;
	}
	else
	if(EndFileFlag==1)
	{
		memset(&WaveFileBuf[WAVEFILEBUFSIZE/2],0,WAVEFILEBUFSIZE/2);
		EndFileFlag=2;
	}	
	else
	if(EndFileFlag==2)
		EndFileFlag=3;
}

uint8_t I2S_WaitFlagStateUntilTimeout(I2S_HandleTypeDef *hi2s, uint32_t Flag, uint32_t Status, uint32_t Timeout)
{
  uint32_t tickstart = 0;
  
  /* Get tick */
  tickstart = HAL_GetTick();
  
  /* Wait until flag is set */
  if(Status == RESET)
  {
    while(__HAL_I2S_GET_FLAG(hi2s, Flag) == RESET)
    {
      if(Timeout != HAL_MAX_DELAY)
      {
        if((Timeout == 0)||((HAL_GetTick() - tickstart ) > Timeout))
        {
          /* Set the I2S State ready */
          hi2s->State= HAL_I2S_STATE_READY;

          /* Process Unlocked */
          __HAL_UNLOCK(hi2s);

          return 1;
        }
      }
    }
  }
  else
  {
    while(__HAL_I2S_GET_FLAG(hi2s, Flag) != RESET)
    {
      if(Timeout != HAL_MAX_DELAY)
      {
        if((Timeout == 0)||((HAL_GetTick() - tickstart ) > Timeout))
        {
          /* Set the I2S State ready */
          hi2s->State= HAL_I2S_STATE_READY;

          /* Process Unlocked */
          __HAL_UNLOCK(hi2s);

          return 1;
        }
      }
    }
  }
  return 0;
}
