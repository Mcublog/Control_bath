/*
 * onewire.c
 *
 *  Created on: 13.02.2012
 *      Author: di
 */

#include "onewire.h"

#ifdef OW_USART1
	#define OW_USART 			USART1
	#define OW_DMA				DMA1
	#define OW_DMA_CH_RX 	DMA1_Channel3
	#define OW_DMA_CH_TX 	DMA1_Channel2
	static UART_HandleTypeDef _huart;//служебная переменная для 
	//инициализации и отправки/приемы данных
	//с помощью функций HAL
#endif

//-----------------------------------------------------------------------------
// функция инициализирует UART_OW
//-----------------------------------------------------------------------------
__inline static void _ow_usart_init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;

	__HAL_RCC_USART1_CLK_ENABLE();
  
	/**USART1 GPIO Configuration    
	PA2     ------> USART1_TX 
	*/
	GPIO_InitStruct.Pin = DQ_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
	GPIO_InitStruct.Pull = GPIO_PULLUP;//GPIO_PULLUP
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF1_USART1;
	HAL_GPIO_Init(DQ_GPIO_Port, &GPIO_InitStruct);
}

//-----------------------------------------------------------------------------
// функция инициализации UART_OW с указанной скоростью
//-----------------------------------------------------------------------------
__inline static void _ow_usart_set_speed(uint32_t speed)
{
	_huart.Instance = OW_USART;
	_huart.Init.BaudRate = speed;
	_huart.Init.WordLength = UART_WORDLENGTH_8B;
	_huart.Init.StopBits = UART_STOPBITS_1;
	_huart.Init.Parity = UART_PARITY_NONE;
	_huart.Init.Mode = UART_MODE_TX_RX;
	_huart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	_huart.Init.OverSampling = UART_OVERSAMPLING_16;
	_huart.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_ENABLE;
	_huart.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
	HAL_HalfDuplex_Init(&_huart);	
}

//-----------------------------------------------------------------------------
// обертка функция отправки по UART 1 байта
//-----------------------------------------------------------------------------
__inline static void _ow_uart_byte_send(uint8_t data)
{
	HAL_UART_Transmit(&_huart, &data, 1, 100);
}
//-----------------------------------------------------------------------------
// обертка функция отправки/приема по UART через DMA
//-----------------------------------------------------------------------------
__inline static uint32_t _ow_uart_dma_send_recv(uint8_t* data, uint32_t cnt)
{
	uint32_t i=0;
	uint32_t status=OW_OK;
	// DMA на чтение
	uart_dma_rx_link(OW_USART,OW_DMA_CH_RX,data,cnt);	
	// DMA на запись
	uart_dma_tx_link(OW_USART,OW_DMA_CH_TX);
	//очистить все флаги
	SET_BIT(OW_DMA->IFCR,DMA_IFCR_CTCIF2);//сбрасываю флаг отправки
	SET_BIT(OW_DMA->IFCR,DMA_IFCR_CTCIF3);//сбрасываю флаг приема	
	
	uart_dma_tx(OW_DMA_CH_TX, data, cnt);
		
	// Ждем, пока не примем 8 байт
		while (READ_BIT(OW_DMA->ISR, DMA_IFCR_CTCIF3)==0)
		{
#ifdef OW_GIVE_TICK_RTOS
			taskYIELD();
#endif
#ifndef OW_GIVE_TICK_RTOS
			HAL_Delay(1);
			i++;
			if(i==1000)
			{
				status=OW_ERROR;
				break;
			}
#endif				
		}
		// отключаем DMA
		//перед отключением нужно очистить все статусные флаги
		SET_BIT(OW_DMA->IFCR,DMA_IFCR_CGIF2);
		SET_BIT(OW_DMA->IFCR,DMA_IFCR_CHTIF2);
		SET_BIT(OW_DMA->IFCR,DMA_IFCR_CTCIF2);
		
		SET_BIT(OW_DMA->IFCR,DMA_IFCR_CGIF3);
		SET_BIT(OW_DMA->IFCR,DMA_IFCR_CHTIF3);	
		SET_BIT(OW_DMA->IFCR,DMA_IFCR_CTCIF3);
		
		CLEAR_BIT(OW_DMA_CH_TX->CCR,DMA_CCR_EN);
		CLEAR_BIT(OW_DMA_CH_RX->CCR,DMA_CCR_EN);

		return status;
}
//-----------------------------------------------------------------------------
// обертка функция приема по UART 1 байта
//-----------------------------------------------------------------------------
__inline static uint8_t _ow_uart_byte_recv(void)
{
	uint8_t data;
	HAL_UART_Receive(&_huart, &data, 1, 100);
	return data;
}

// Буфер для приема/передачи по 1-wire
uint8_t ow_buf[8];

#define OW_0	  0x00
#define OW_1	  0xff
#define OW_R_1	0xff

//-----------------------------------------------------------------------------
// функция преобразует один байт в восемь, для передачи через USART
// ow_byte - байт, который надо преобразовать
// ow_bits - ссылка на буфер, размером не менее 8 байт
//-----------------------------------------------------------------------------
void OW_toBits(uint8_t ow_byte, uint8_t *ow_bits) {
	uint8_t i;
	for (i = 0; i < 8; i++) {
		if (ow_byte & 0x01) {
			*ow_bits = OW_1;
		} else {
			*ow_bits = OW_0;
		}
		ow_bits++;
		ow_byte = ow_byte >> 1;
	}
}

//-----------------------------------------------------------------------------
// обратное преобразование - из того, что получено через USART опять собирается байт
// ow_bits - ссылка на буфер, размером не менее 8 байт
//-----------------------------------------------------------------------------
uint8_t OW_toByte(uint8_t *ow_bits) {
	uint8_t ow_byte, i;
	ow_byte = 0;
	for (i = 0; i < 8; i++) {
		ow_byte = ow_byte >> 1;
		if (*ow_bits == OW_R_1) {
			ow_byte |= 0x80;
		}
		ow_bits++;
	}

	return ow_byte;
}

//-----------------------------------------------------------------------------
// инициализирует USART
//-----------------------------------------------------------------------------
uint8_t OW_Init(void) 
{
	_ow_usart_init();
	return OW_OK;
}
//-----------------------------------------------------------------------------
// осуществляет сброс и проверку на наличие устройств на шине
//-----------------------------------------------------------------------------
uint8_t OW_Reset(void) {
	uint8_t ow_presence;
	_ow_usart_set_speed(9600);
	// отправляем 0xf0 на скорости 9600
	uint8_t data=0xf0;
	_ow_uart_byte_send(data);
	ow_presence=_ow_uart_byte_recv();
	_ow_usart_set_speed(115200);

	if (ow_presence != 0xf0) {
		return OW_OK;
	}

	return OW_NO_DEVICE;
}


//-----------------------------------------------------------------------------
// процедура общения с шиной 1-wire
// sendReset - посылать RESET в начале общения.
// 		OW_SEND_RESET или OW_NO_RESET
// command - массив байт, отсылаемых в шину. Если нужно чтение - отправляем OW_READ_SLOTH
// cLen - длина буфера команд, столько байт отошлется в шину
// data - если требуется чтение, то ссылка на буфер для чтения
// dLen - длина буфера для чтения. Прочитается не более этой длины
// readStart - с какого символа передачи начинать чтение (нумеруются с 0)
//		можно указать OW_NO_READ, тогда можно не задавать data и dLen
//-----------------------------------------------------------------------------
uint8_t OW_Send(uint8_t sendReset, uint8_t *command, uint8_t cLen,
		uint8_t *data, uint8_t dLen, uint8_t readStart) 
{
	volatile uint32_t i=0;
	// если требуется сброс - сбрасываем и проверяем на наличие устройств
	if (sendReset == OW_SEND_RESET) 
	{
		if (OW_Reset() == OW_NO_DEVICE) return OW_NO_DEVICE;
	}
	while (cLen > 0) 
	{

		OW_toBits(*command, ow_buf);
		command++;
		cLen--;
		
		if (_ow_uart_dma_send_recv(ow_buf, 8)==OW_ERROR) return OW_ERROR;
		
		// если прочитанные данные кому-то нужны - выкинем их в буфер
		if (readStart == 0 && dLen > 0) {
			*data = OW_toByte(ow_buf);
			data++;
			dLen--;
		} else {
			if (readStart != OW_NO_READ) {
				readStart--;
			}
		}
	}

	return OW_OK;
}

