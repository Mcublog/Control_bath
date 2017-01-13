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
	static UART_HandleTypeDef _huart;//��������� ���������� ��� 
	//������������� � ��������/������ ������
	//� ������� ������� HAL
#endif

//-----------------------------------------------------------------------------
// ������� �������������� UART_OW
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
// ������� ������������� UART_OW � ��������� ���������
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
// ������� ������� �������� �� UART 1 �����
//-----------------------------------------------------------------------------
__inline static void _ow_uart_byte_send(uint8_t data)
{
	HAL_UART_Transmit(&_huart, &data, 1, 100);
}
//-----------------------------------------------------------------------------
// ������� ������� ��������/������ �� UART ����� DMA
//-----------------------------------------------------------------------------
__inline static uint32_t _ow_uart_dma_send_recv(uint8_t* data, uint32_t cnt)
{
	uint32_t i=0;
	uint32_t status=OW_OK;
	// DMA �� ������
	uart_dma_rx_link(OW_USART,OW_DMA_CH_RX,data,cnt);	
	// DMA �� ������
	uart_dma_tx_link(OW_USART,OW_DMA_CH_TX);
	//�������� ��� �����
	SET_BIT(OW_DMA->IFCR,DMA_IFCR_CTCIF2);//��������� ���� ��������
	SET_BIT(OW_DMA->IFCR,DMA_IFCR_CTCIF3);//��������� ���� ������	
	
	uart_dma_tx(OW_DMA_CH_TX, data, cnt);
		
	// ����, ���� �� ������ 8 ����
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
		// ��������� DMA
		//����� ����������� ����� �������� ��� ��������� �����
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
// ������� ������� ������ �� UART 1 �����
//-----------------------------------------------------------------------------
__inline static uint8_t _ow_uart_byte_recv(void)
{
	uint8_t data;
	HAL_UART_Receive(&_huart, &data, 1, 100);
	return data;
}

// ����� ��� ������/�������� �� 1-wire
uint8_t ow_buf[8];

#define OW_0	  0x00
#define OW_1	  0xff
#define OW_R_1	0xff

//-----------------------------------------------------------------------------
// ������� ����������� ���� ���� � ������, ��� �������� ����� USART
// ow_byte - ����, ������� ���� �������������
// ow_bits - ������ �� �����, �������� �� ����� 8 ����
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
// �������� �������������� - �� ����, ��� �������� ����� USART ����� ���������� ����
// ow_bits - ������ �� �����, �������� �� ����� 8 ����
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
// �������������� USART
//-----------------------------------------------------------------------------
uint8_t OW_Init(void) 
{
	_ow_usart_init();
	return OW_OK;
}
//-----------------------------------------------------------------------------
// ������������ ����� � �������� �� ������� ��������� �� ����
//-----------------------------------------------------------------------------
uint8_t OW_Reset(void) {
	uint8_t ow_presence;
	_ow_usart_set_speed(9600);
	// ���������� 0xf0 �� �������� 9600
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
// ��������� ������� � ����� 1-wire
// sendReset - �������� RESET � ������ �������.
// 		OW_SEND_RESET ��� OW_NO_RESET
// command - ������ ����, ���������� � ����. ���� ����� ������ - ���������� OW_READ_SLOTH
// cLen - ����� ������ ������, ������� ���� ��������� � ����
// data - ���� ��������� ������, �� ������ �� ����� ��� ������
// dLen - ����� ������ ��� ������. ����������� �� ����� ���� �����
// readStart - � ������ ������� �������� �������� ������ (���������� � 0)
//		����� ������� OW_NO_READ, ����� ����� �� �������� data � dLen
//-----------------------------------------------------------------------------
uint8_t OW_Send(uint8_t sendReset, uint8_t *command, uint8_t cLen,
		uint8_t *data, uint8_t dLen, uint8_t readStart) 
{
	volatile uint32_t i=0;
	// ���� ��������� ����� - ���������� � ��������� �� ������� ���������
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
		
		// ���� ����������� ������ ����-�� ����� - ������� �� � �����
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

