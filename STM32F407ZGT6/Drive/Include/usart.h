#ifndef __USART_H
#define __USART_H

#include "stdio.h"
#include "stm32f4xx.h"

/*----------------------USART���ú� ------------------------*/

#define  USART1_BaudRate  115200

#define  USART1_TX_PIN				GPIO_Pin_9					// TX ����
#define	USART1_TX_PORT				GPIOA							// TX ���Ŷ˿�
#define	USART1_TX_CLK				RCC_AHB1Periph_GPIOA		// TX ����ʱ��
#define  USART1_TX_PinSource     GPIO_PinSource9			// ����Դ

#define  USART1_RX_PIN				GPIO_Pin_10             // RX ����
#define	USART1_RX_PORT				GPIOA                   // RX ���Ŷ˿�
#define	USART1_RX_CLK				RCC_AHB1Periph_GPIOA    // RX ����ʱ��
#define  USART1_RX_PinSource     GPIO_PinSource10        // ����Դ

#define K230_RX_BUFFER_SIZE 80

/*---------------------- �������� ----------------------------*/

void  Usart_Config (void);	// USART��ʼ������
void  K230_UartRxHandler(void);
uint8_t K230_ReadLine(char *line, uint16_t max_len);

#endif //__USART_H
