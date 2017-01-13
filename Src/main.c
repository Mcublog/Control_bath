/**
  ******************************************************************************
  * File Name          : main.c
  * Description        : Main program body
  ******************************************************************************
**/

/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx_hal.h"

#include "init_main.h"
#include "onewire.h"

//**************макросы, глобальные переменные, прототипы функций******************
UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart1_tx;
TIM_HandleTypeDef htim16;

#define WORK_TIME (20)

uint32_t status=0;
uint32_t temp=0;
uint32_t limit=0;
uint32_t work_time=25;
	
uint8_t buf[2];
uint16_t* t=(uint16_t*)buf;

int main(void)
{
//******************************Инициализация**************************************
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_DMA_Init();
	MX_TIM16_Init();
//*********************************************************************************	

	status= OW_Init();

  while (1)
  {
		OW_Send(OW_SEND_RESET, (uint8_t*)"\xcc\x44", 2, NULL, NULL, OW_NO_READ);
		HAL_Delay(1000);
		status=OW_Send(OW_SEND_RESET,(uint8_t*) "\xcc\xbe\xff\xff", 4, buf,2, 2);
		if (status==OW_OK)
		{
			HAL_GPIO_WritePin(GPIOF, LED_Pin,GPIO_PIN_RESET);
			t=(uint16_t*)buf;
			temp=*t*0.0625;

			if (temp>=35)
			{
				HAL_GPIO_WritePin(GPIOA, REL0_Pin|REL1_Pin, GPIO_PIN_SET);	
				limit=0;
			}
			else 
			{
				limit++;
				if 			(limit<work_time) 	HAL_GPIO_WritePin(GPIOA, REL0_Pin|REL1_Pin, GPIO_PIN_RESET);
				else if (limit==work_time)	HAL_GPIO_WritePin(GPIOA, REL0_Pin|REL1_Pin, GPIO_PIN_SET);
				else if	(limit>=work_time*3)limit=0;
			}
		}
		else HAL_GPIO_WritePin(GPIOF, LED_Pin,GPIO_PIN_SET);
  }
}
