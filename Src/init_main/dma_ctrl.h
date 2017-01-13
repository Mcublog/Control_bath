#ifndef DMA_CTRL_H
#define DMA_CTRL_H

#include "stm32f0xx_hal.h"

void uart_dma_tx_link(USART_TypeDef * USART,DMA_Channel_TypeDef * DMA_Channel);
void uart_dma_rx_link(USART_TypeDef * USART,DMA_Channel_TypeDef * DMA_Channel, uint8_t* data, uint16_t len);
void uart_dma_tx(DMA_Channel_TypeDef * DMA_Channel, uint8_t* data, uint16_t len);
void uart_dma_rx(DMA_Channel_TypeDef * DMA_Channel, uint8_t* data, uint16_t len);//лен больше чем предоплагаемая дата(конец про прерыванию IDLE USART)

#endif
