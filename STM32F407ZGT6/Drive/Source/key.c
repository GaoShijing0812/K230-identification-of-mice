/***
	*************************************************************************************************
	*	@version V1.0
	*	@author  鹿小班科技	
	*	@brief   按键接口相关函数
   *************************************************************************************************
   *  @description
	*
	*	实验平台：鹿小班STM32F407ZGT6核心板 （型号：LXB407ZG-P1）
	* 客服微信：19949278543
	*
>>>>> 文件说明：
	*
	*	初始化按键引脚，配置为上拉输入、速度等级2M。
	*
	************************************************************************************************
***/

#include "key.h"  
/***********************************************************
@函数名：KEY_Init
@入口参数：无

功能描述：初始化按键对应的GPIO端口，配置为输入模式、上拉、2MHz速率
@作者：单雁
@日期：
*************************************************************/
void KEY_Init(void)
{		
	GPIO_InitTypeDef GPIO_InitStructure; //定义结构体
	RCC_AHB1PeriphClockCmd ( KEY_CLK, ENABLE); 	//初始化KEY时钟	
	
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;   //输出模式
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;	//上拉
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz; //速度选择
	GPIO_InitStructure.GPIO_Pin   = KEY_PIN;	 
	
	GPIO_Init(KEY_PORT, &GPIO_InitStructure);	

}
/***********************************************************
@函数名：KEY_Scan
@入口参数：无

功能描述：扫描按键状态，检测到按键按下并松开后返回KEY_ON
@作者：单雁
@日期：
*************************************************************/
uint8_t	KEY_Scan(void)
{
	if( GPIO_ReadInputDataBit ( KEY_PORT,KEY_PIN) == 0 )	//检测按键是否被按下
	{	
		Delay_ms(10);	//延时消抖
		if(GPIO_ReadInputDataBit ( KEY_PORT,KEY_PIN) == 0)	//再次检测是否为低电平
		{
			while(GPIO_ReadInputDataBit ( KEY_PORT,KEY_PIN) == 0);	//等待按键放开
			return KEY_ON;	//返回按键按下标志
		}
	}
	return KEY_OFF;	
}


