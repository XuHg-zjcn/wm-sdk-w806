/******************************************************************************
** 
 * \file        main.c
 * \author      IOsetting | iosetting@outlook.com
 * \date        
 * \brief       Demo code of PWM in independent mode
 * \note        This will drive 3 on-board LEDs to show fade effect
 * \version     v0.1
 * \ingroup     demo
 * \remarks     test-board: HLK-W806-KIT-V1.0
 *              PWM Frequency = 40MHz / Prescaler / (Period + 1)；
                Duty Cycle(Edge Aligned)   = (Pulse + 1) / (Period + 1)
                Duty Cycle(Center Aligned) = (2 * Pulse + 1) / (2 * (Period + 1))
 *
******************************************************************************/

#include <stdio.h>
#include "wm_hal.h"
#include "ff.h"

#define TEST_DEBUG printf

SPI_HandleTypeDef hspi;

void Error_Handler(void);

uint8_t SdSpiReadWriteByte(uint8_t write_byte)
{
    uint8_t read_byte;
    HAL_SPI_TransmitReceive(&hspi, &write_byte, &read_byte, 1, 1000);
    return read_byte;
}

static void SPI_Init(void)
{
    hspi.Instance = SPI;
    hspi.Init.Mode = SPI_MODE_MASTER;
    hspi.Init.CLKPolarity = SPI_POLARITY_HIGH;
    hspi.Init.CLKPhase = SPI_PHASE_2EDGE;
    hspi.Init.NSS = SPI_NSS_SOFT;
    hspi.Init.BaudRatePrescaler = 59;
    hspi.Init.FirstByte = SPI_LITTLEENDIAN;
    
    if (HAL_SPI_Init(&hspi) != HAL_OK)
    {
        Error_Handler();
    }
}

void SdIOInit(void)
{
    //SPI和CS引脚会在SPI_Init中初始化,这里就不用了
	//仍然保留这个函数, spisd.c里要调用
}

void Write_CS_Pin(int x)
{
    if(x){
        __HAL_SPI_SET_CS_HIGH(&hspi);
    }else{
        __HAL_SPI_SET_CS_LOW(&hspi);
    }
}

void SdSpiSpeedLow(void)
{
    hspi.Init.BaudRatePrescaler = 60 - 1;
    if (HAL_SPI_Init(&hspi) != HAL_OK)
    {
        Error_Handler();
    }
}

void SdSpiSpeedHigh(void)
{
    hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    if (HAL_SPI_Init(&hspi) != HAL_OK)
    {
        Error_Handler();
    }
}

//FAT功能测试：格式化测试，文件写入测试，文件读取测试（基本功能）
//目前有点小问题，不清楚原因。有时候复位后会卡住，需要将SD卡断电后再上电才能识别。
int fatfs_func(void)
{
	FATFS fs; //FatFs文件系统对象
	FIL fnew; //文件对象
	FRESULT res_sd;//文件操作结果
	UINT fnum; //文件成功读写数量
	BYTE ReadBuffer[256] = {0};
	BYTE work[FF_MAX_SS];
	BYTE WriteBuffer[] = "成功移植了FatFs文件系统！\r\n"; //写缓存区
	uint32_t Tick0, cost_ms;
	

	//挂载SD卡
	res_sd = f_mount(&fs, "1:/", 1);
	
	//***********************格式化测试****************************
	if(res_sd == FR_NO_FILESYSTEM)
	{
		while(1)
		{
			TEST_DEBUG("SD卡没有文件系统，即将进行格式化...\r\n");
			//格式化
			
			res_sd = f_mkfs("1:", 0, work, sizeof(work));
			
			if(res_sd == FR_OK)
			{
				TEST_DEBUG("SD卡成功格式化！\r\n");
				//格式化后先取消挂载
				res_sd = f_mount(NULL, "1:", 1);
				//再重新挂载
				res_sd = f_mount(&fs, "1:", 1);
				break;
			}
		}
	}
	else if(res_sd != FR_OK)
	{
		TEST_DEBUG("挂载文件系统失败！可能是因为文件初始化失败！错误代码：%d\r\n", res_sd);
	}
	else
	{
		TEST_DEBUG("文件系统挂载成功， 可进行读写测试！\r\n");
	}
	
	//***********************写测试****************************
	//打开文件，如果文件不存在则创建它
	TEST_DEBUG("即将进行文件写入测试....\r\n");
	//打开文件，若不存在就创建
	res_sd = f_open(&fnew, "1:/fatfs.txt", FA_CREATE_ALWAYS | FA_WRITE);
	//文件打开成功
	if(res_sd == FR_OK)
	{
		TEST_DEBUG("打开文件成功！开始写入数据！\r\n");
		res_sd= f_write(&fnew, WriteBuffer, strlen(WriteBuffer), &fnum);
		
		if(res_sd == FR_OK)
		{
			TEST_DEBUG("数据写入成功，共写入%d个字符！\r\n", fnum);
			TEST_DEBUG("数据：%s", WriteBuffer);
		}
		else
		{
			TEST_DEBUG("数据写入失败！\r\n");
		}
		
		//关闭文件
		f_close(&fnew);
	}else{
		TEST_DEBUG("文件打开失败！错误码=%d\r\n", res_sd);
	}
	
	//***********************读测试****************************
	//请在电脑上创建read.txt文件，写点内容供测试
	//打开文件，如果文件不存在则打开刚才写入的文件
	TEST_DEBUG("即将进行文件读取测试....\r\n");
	res_sd = f_open(&fnew, "1:/read.txt", FA_OPEN_EXISTING | FA_READ);
	if(res_sd != FR_OK)
	{
		TEST_DEBUG("无法打开read.txt！错误码=%d, 打开刚才写入的文件.\r\n", res_sd);
		res_sd = f_open(&fnew, "1:/fatfs.txt", FA_OPEN_EXISTING | FA_READ);
	}
	//文件打开成功
	if(res_sd == FR_OK)
	{
		TEST_DEBUG("打开文件成功！开始读取数据！\r\n");
		res_sd= f_read(&fnew, ReadBuffer, sizeof(ReadBuffer), &fnum);
		
		if(res_sd == FR_OK)
		{
			TEST_DEBUG("数据读取成功！\r\n");
			TEST_DEBUG("数据：%s\r\n", ReadBuffer);
		}
		else
		{
			TEST_DEBUG("数据读取失败！\r\n");
		}
		
		//关闭文件
		f_close(&fnew);
	}else{
		TEST_DEBUG("文件打开失败！错误码=%d\r\n", res_sd);
	}

	TEST_DEBUG("即将进行写入速度测试....\r\n");
	//打开文件，若不存在就创建
	res_sd = f_open(&fnew, "1:/speed_test.txt", FA_CREATE_ALWAYS | FA_WRITE);
	//文件打开成功
	if(res_sd == FR_OK)
	{
		uint32_t bytes = 0;
		Tick0 = HAL_GetTick();
		for(uint32_t i=0;i<1024;i++)
		{
			f_write(&fnew, WriteBuffer, strlen(WriteBuffer), &fnum);
			bytes += fnum;
		}
		f_close(&fnew);
		cost_ms = HAL_GetTick() - Tick0;
		printf("写入%dkiB, 耗时%dms, 平均%dkiB/s\r\n", \
		       bytes/1024, cost_ms, (bytes*1000/1024)/cost_ms);
	}else{
		TEST_DEBUG("文件打开失败！错误码=%d\r\n", res_sd);
	}

	TEST_DEBUG("即将进行读速度测试....\r\n");
	//打开刚写入的测试文件
	res_sd = f_open(&fnew, "1:/speed_test.txt", FA_OPEN_EXISTING | FA_READ);
	//文件打开成功
	if(res_sd == FR_OK)
	{
		uint32_t bytes = 0;
		Tick0 = HAL_GetTick();
		fnum = 1;
		while(fnum != 0)
		{
			f_read(&fnew, ReadBuffer, sizeof(ReadBuffer), &fnum);
			bytes += fnum;
		}
		f_close(&fnew);
		cost_ms = HAL_GetTick() - Tick0;
		printf("读出%dkiB, 耗时%dms, 平均%dkB/s\r\n", \
		       bytes/1024, cost_ms, bytes/cost_ms);
	}else{
		TEST_DEBUG("文件打开失败！错误码=%d\r\n", res_sd);
	}

	//取消挂载文件系统
	f_mount(NULL, "1:", 1);

	return 0;
}

int main(void)
{
    SystemClock_Config(CPU_CLK_160M);
    printf("enter main\r\n");
    SPI_Init();
    fatfs_func();
    while (1);
}

void Error_Handler(void)
{
    while (1)
    {
    }
}

void assert_failed(uint8_t *file, uint32_t line)
{
    printf("Wrong parameters value: file %s on line %d\r\n", file, line);
}