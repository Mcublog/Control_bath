#include "dma_ctrl.h"
//**************************************************************************
void uart_dma_tx_link(USART_TypeDef * USART,DMA_Channel_TypeDef * DMA_Channel)
{
  CLEAR_BIT(DMA_Channel->CCR, DMA_CCR_EN);//
  //SET_BIT(DMA_Channel->CCR, DMA_CCR_TCIE);//прерывание по завершению передачи
  SET_BIT(DMA_Channel->CCR, DMA_CCR_MINC);//инкремент памяти
  SET_BIT(DMA_Channel->CCR, DMA_CCR_DIR);//из памяти в переферию
  
  WRITE_REG(DMA_Channel->CPAR, (uint32_t)&(USART->TDR));
    
  SET_BIT(USART->CR3, USART_CR3_DMAT );//разрешить работу ДМА на передачу  
}

void uart_dma_rx_link(USART_TypeDef * USART,DMA_Channel_TypeDef * DMA_Channel, uint8_t* data, uint16_t len) 
{
  CLEAR_BIT(DMA_Channel->CCR, DMA_CCR_EN);//
  SET_BIT(DMA_Channel->CCR, DMA_CCR_MINC);//инкремент памяти
  CLEAR_BIT(DMA_Channel->CCR, DMA_CCR_DIR);//из переферии в память
  
  WRITE_REG(DMA_Channel->CNDTR, len);
  WRITE_REG(DMA_Channel->CMAR, (uint32_t)data);
  WRITE_REG(DMA_Channel->CPAR, (uint32_t)&(USART->RDR));
  SET_BIT(DMA_Channel->CCR, DMA_CCR_EN);//
  
  //SET_BIT(USART->CR1, USART_CR1_IDLEIE );//прерывание IDLE
  SET_BIT(USART->CR3, USART_CR3_DMAR );//разрешить работу дма на прием  
}

void uart_dma_tx(DMA_Channel_TypeDef * DMA_Channel, uint8_t* data, uint16_t len)
{
	
  CLEAR_BIT(DMA_Channel->CCR, DMA_CCR_EN);//
  
  WRITE_REG(DMA_Channel->CNDTR, len);
  WRITE_REG(DMA_Channel->CMAR, (uint32_t)data);
  
  SET_BIT(DMA_Channel->CCR, DMA_CCR_EN);//  
}

void uart_dma_rx(DMA_Channel_TypeDef * DMA_Channel, uint8_t* data, uint16_t len)
{
  CLEAR_BIT(DMA_Channel->CCR, DMA_CCR_EN);//
  WRITE_REG(DMA_Channel->CNDTR, len);//кол-во можно любое, окончание приема определяется по IRQ UART IDLE
  WRITE_REG(DMA_Channel->CMAR, (uint32_t)&data[0]);
  SET_BIT(DMA_Channel->CCR, DMA_CCR_EN);//  
}
