#ifndef INIT_MAIN_H
#define INIT_MAIN_H

#include "stm32f0xx_hal.h"


extern UART_HandleTypeDef huart1;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart1_tx;
extern TIM_HandleTypeDef htim16;

void SystemClock_Config(void);
void Error_Handler(void);
void MX_GPIO_Init(void);
void MX_DMA_Init(void);
void MX_USART1_UART_Init(void);
void MX_TIM16_Init(void);

#endif
