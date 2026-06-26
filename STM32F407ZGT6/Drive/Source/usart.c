#include "usart.h"

static volatile char k230_line_buffer[K230_RX_BUFFER_SIZE];
static volatile char k230_ready_line[K230_RX_BUFFER_SIZE];
static volatile uint16_t k230_line_index = 0;
static volatile uint8_t k230_line_ready = 0;
/***********************************************************
@函数名：USART_GPIO_Config
@入口参数：无

功能描述：初始化USART1的TX/RX引脚GPIO，配置为复用功能
@作者：单雁
@日期：
*************************************************************/
void  USART_GPIO_Config	(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_AHB1PeriphClockCmd ( USART1_TX_CLK|USART1_RX_CLK, ENABLE); 	//IO口时钟使能

	//IO配置
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;   	 //复用模式
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;   //推挽
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;		 //上拉
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz; //速度等级

	//初始化 TX	引脚
	GPIO_InitStructure.GPIO_Pin = USART1_TX_PIN;	 
	GPIO_Init(USART1_TX_PORT, &GPIO_InitStructure);	
	//初始化 RX 引脚													   
	GPIO_InitStructure.GPIO_Pin = USART1_RX_PIN;	
	GPIO_Init(USART1_RX_PORT, &GPIO_InitStructure);		
	
	//IO复用，映射到USART1
	GPIO_PinAFConfig(USART1_TX_PORT,USART1_TX_PinSource,GPIO_AF_USART1); 
	GPIO_PinAFConfig(USART1_RX_PORT,USART1_RX_PinSource,GPIO_AF_USART1);	
}
/***********************************************************
@函数名：Usart_Config
@入口参数：无

功能描述：初始化USART1串口参数，使能接收中断和NVIC
@作者：单雁
@日期：
*************************************************************/
void Usart_Config(void)
{		
	USART_InitTypeDef USART_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	
	// IO口初始化
	USART_GPIO_Config();
		 
	// 配置串口各参数
	USART_InitStructure.USART_BaudRate 	 = USART1_BaudRate; //波特率
	USART_InitStructure.USART_WordLength = USART_WordLength_8b; //数据位8位
	USART_InitStructure.USART_StopBits   = USART_StopBits_1; //停止位1位
	USART_InitStructure.USART_Parity     = USART_Parity_No ; //无校验
	USART_InitStructure.USART_Mode 	    = USART_Mode_Rx | USART_Mode_Tx; //发送和接收模式
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 不使用硬件流控制
	
	USART_Init(USART1,&USART_InitStructure); //初始化串口1
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	NVIC_EnableIRQ(USART1_IRQn);
	USART_Cmd(USART1,ENABLE);	//使能串口1
}
/***********************************************************
@函数名：K230_UartRxHandler
@入口参数：无

功能描述：USART1接收中断服务函数，缓存接收字符并组装成完整数据行
@作者：单雁
@日期：
*************************************************************/
void K230_UartRxHandler(void)
{
	uint8_t data;
	uint16_t i;

	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	{
		data = (uint8_t)USART_ReceiveData(USART1);
		USART_ClearITPendingBit(USART1, USART_IT_RXNE);

		if(data == '\r')
		{
			return;
		}

		if(data == '\n')
		{
			if(k230_line_index > 0)
			{
				for(i = 0; i < k230_line_index; i++)
				{
					k230_ready_line[i] = k230_line_buffer[i];
				}
				k230_ready_line[k230_line_index] = '\0';
				k230_line_ready = 1;
				k230_line_index = 0;
			}
			return;
		}

		if(k230_line_index < K230_RX_BUFFER_SIZE - 1)
		{
			k230_line_buffer[k230_line_index++] = (char)data;
		}
		else
		{
			k230_line_index = 0;
		}
	}
}
/***********************************************************
@函数名：K230_ReadLine
@入口参数：char *line,//读取缓冲区指针
					 uint16_t max_len,//缓冲区最大长度

功能描述：从串口接收缓存中读取一行完整数据
@作者：单雁
@日期：
*************************************************************/
uint8_t K230_ReadLine(char *line, uint16_t max_len)
{
	uint16_t i;

	if(k230_line_ready == 0 || max_len == 0)
	{
		return 0;
	}

	__disable_irq();
	for(i = 0; i < max_len - 1 && k230_ready_line[i] != '\0'; i++)
	{
		line[i] = k230_ready_line[i];
	}
	line[i] = '\0';
	k230_line_ready = 0;
	__enable_irq();

	return 1;
}
/***********************************************************
@函数名：fputc
@入口参数：int c,//要发送的字符
					 FILE *fp,//文件指针

功能描述：重定向printf输出到USART1串口
@作者：单雁
@日期：
*************************************************************/
int fputc(int c, FILE *fp)
{

	USART_SendData( USART1,(u8)c );	// 发送单个字节数据
	while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);	//等待发送完成 

	return (c); //返回字符
}


