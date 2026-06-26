/***
	*************************************************************************************************
	*	@version V1.0
	*	@author  鹿小班科技	
	*	@brief   LED接口相关
   *************************************************************************************************
   *  @description
	*
	*	实验平台：鹿小班STM32F407ZGT6核心板 （型号：LXB407ZG-P1）
	* 客服微信：19949278543
	*
>>>>> 文件说明：
	*
	*	初始化LED的IO口，配置为推挽输出、速度等级2M。
	*
	************************************************************************************************
***/


#include "led.h"  
/***********************************************************
@函数名：LED_Init
@入口参数：无

功能描述：初始化LED对应的GPIO端口，配置为推挽输出、上拉、2MHz速率
@作者：单雁
@日期：
*************************************************************/
void LED_Init(void)
{		
	GPIO_InitTypeDef GPIO_InitStructure; //定义结构体
	RCC_AHB1PeriphClockCmd ( LED1_CLK, ENABLE); 	//初始化GPIOG时钟	
	
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;   //输出模式
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;  //推挽输出
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;	//上拉
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz; //速度选择
	
	//初始化 LED1 引脚
	GPIO_InitStructure.GPIO_Pin = LED1_PIN;	 
	GPIO_Init(LED1_PORT, &GPIO_InitStructure);	
	
	GPIO_ResetBits(LED1_PORT,LED1_PIN);  //PG7输出低电平
}




