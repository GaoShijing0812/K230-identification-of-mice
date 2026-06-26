/***
	***************************************************************************************************
	* @file main.c
	* @brief K230 UART mouse status display for STM32F407ZGT6 + ST7789 240x240 LCD
	***************************************************************************************************
***/

#include "stm32f4xx.h"
#include "led.h"
#include "delay.h"
#include "key.h"
#include "usart.h"
#include "lcd_spi_154.h"

#define MOUSE_MAX_COUNT 3
#define K230_LINE_SIZE  K230_RX_BUFFER_SIZE

typedef enum
{
	MOUSE_STATUS_UNKNOWN = 0,
	MOUSE_STATUS_EATING = 1,
	MOUSE_STATUS_SLEEPING = 2,
	MOUSE_STATUS_RUNNING = 3
} MouseStatus;

typedef struct
{
	uint8_t total;
	uint8_t eating;
	uint8_t sleeping;
	uint8_t running;
	MouseStatus mouse[MOUSE_MAX_COUNT];
} MouseData;

static MouseData g_mouse_data = {0, 0, 0, 0, {MOUSE_STATUS_UNKNOWN, MOUSE_STATUS_UNKNOWN, MOUSE_STATUS_UNKNOWN}};
static uint8_t g_current_page = 0;
static uint8_t g_lcd_need_refresh = 1;

static uint8_t K230_ParseMouseData(char *line, MouseData *data);
static uint8_t K230_ParseStatus(char *token, MouseStatus *status);
static char *K230_NextToken(char **text);
static int32_t K230_ParseNumber(char *token);
static char *MouseStatusText(MouseStatus status);
static uint32_t MouseStatusColor(MouseStatus status);
static void MouseDisplay_Refresh(MouseData *data, uint8_t page);
static void MouseDisplay_DrawSummary(MouseData *data);
static void MouseDisplay_DrawMouse(MouseData *data, uint8_t mouse_index);
static void MouseDisplay_DrawHeader(char *title, uint8_t page);
static void MouseDisplay_DrawLine(char *label, int32_t value, uint16_t y, uint32_t color);
static void MouseDisplay_DrawStatus(MouseStatus status, uint16_t y);
/***********************************************************
@函数名：main
@入口参数：无

功能描述：程序入口，初始化延时、LED、按键、串口和LCD，循环处理K230数据、按键和显示刷新
@作者：单雁
@日期：
*************************************************************/
int main(void)
{
	char k230_line[K230_LINE_SIZE];

	Delay_Init();
	LED_Init();
	KEY_Init();
	Usart_Config();
	SPI_LCD_Init();

	LCD_SetDirection(Direction_V);
	LCD_SetBackColor(LCD_BLACK);
	LCD_Clear();

	while (1)
	{
		if(K230_ReadLine(k230_line, K230_LINE_SIZE))
		{
			if(K230_ParseMouseData(k230_line, &g_mouse_data))
			{
				g_lcd_need_refresh = 1;
			}
		}

		if(KEY_Scan() == KEY_ON)
		{
			g_current_page++;
			if(g_current_page >= 4)
			{
				g_current_page = 0;
			}
			g_lcd_need_refresh = 1;
		}

		if(g_lcd_need_refresh)
		{
			MouseDisplay_Refresh(&g_mouse_data, g_current_page);
			g_lcd_need_refresh = 0;
		}
	}
}
/***********************************************************
@函数名：K230_ParseMouseData
@入口参数：char *line,//K230串口发送的一行数据
					 MouseData *data,//解析后的鼠标数据结构体指针

功能描述：解析K230串口传来的鼠标状态数据字符串
@作者：单雁
@日期：
*************************************************************/
static uint8_t K230_ParseMouseData(char *line, MouseData *data)
{
	char *cursor = line;
	char *token;
	MouseData temp;
	uint8_t i;

	token = K230_NextToken(&cursor);
	if(token == 0)
	{
		return 0;
	}
	temp.total = (uint8_t)K230_ParseNumber(token);

	token = K230_NextToken(&cursor);
	if(token == 0)
	{
		return 0;
	}
	temp.eating = (uint8_t)K230_ParseNumber(token);

	token = K230_NextToken(&cursor);
	if(token == 0)
	{
		return 0;
	}
	temp.sleeping = (uint8_t)K230_ParseNumber(token);

	token = K230_NextToken(&cursor);
	if(token == 0)
	{
		return 0;
	}
	temp.running = (uint8_t)K230_ParseNumber(token);

	for(i = 0; i < MOUSE_MAX_COUNT; i++)
	{
		token = K230_NextToken(&cursor);
		if(token == 0 || K230_ParseStatus(token, &temp.mouse[i]) == 0)
		{
			temp.mouse[i] = MOUSE_STATUS_UNKNOWN;
		}
	}

	if(temp.total > MOUSE_MAX_COUNT)
	{
		temp.total = MOUSE_MAX_COUNT;
	}
	if(temp.eating > MOUSE_MAX_COUNT)
	{
		temp.eating = MOUSE_MAX_COUNT;
	}
	if(temp.sleeping > MOUSE_MAX_COUNT)
	{
		temp.sleeping = MOUSE_MAX_COUNT;
	}
	if(temp.running > MOUSE_MAX_COUNT)
	{
		temp.running = MOUSE_MAX_COUNT;
	}

	*data = temp;
	return 1;
}
/***********************************************************
@函数名：K230_ParseStatus
@入口参数：char *token,//状态字段字符串
					 MouseStatus *status,//解析得到的状态枚举指针

功能描述：将单个字符或字符串解析为对应的鼠标状态枚举
@作者：单雁
@日期：
*************************************************************/
static uint8_t K230_ParseStatus(char *token, MouseStatus *status)
{
	if(token[0] == '1' || token[0] == 'E' || token[0] == 'e')
	{
		*status = MOUSE_STATUS_EATING;
		return 1;
	}
	if(token[0] == '2' || token[0] == 'S' || token[0] == 's')
	{
		*status = MOUSE_STATUS_SLEEPING;
		return 1;
	}
	if(token[0] == '3' || token[0] == 'R' || token[0] == 'r')
	{
		*status = MOUSE_STATUS_RUNNING;
		return 1;
	}
	if(token[0] == '0' || token[0] == 'U' || token[0] == 'u' || token[0] == 'N' || token[0] == 'n')
	{
		*status = MOUSE_STATUS_UNKNOWN;
		return 1;
	}
	return 0;
}
/***********************************************************
@函数名：K230_NextToken
@入口参数：char **text,//指向待解析字符串的指针，解析后前移

功能描述：从字符串中按逗号、分号或空白分隔提取下一个字段
@作者：单雁
@日期：
*************************************************************/
static char *K230_NextToken(char **text)
{
	char *start = *text;

	while(*start == ' ' || *start == '\t')
	{
		start++;
	}

	if(*start == '\0')
	{
		*text = start;
		return 0;
	}

	*text = start;
	while(**text != ',' && **text != ';' && **text != ' ' && **text != '\t' && **text != '\0')
	{
		(*text)++;
	}

	if(**text != '\0')
	{
		**text = '\0';
		(*text)++;
	}

	return start;
}
/***********************************************************
@函数名：K230_ParseNumber
@入口参数：char *token,//数字字符串

功能描述：将连续数字字符组成的字符串转换为整数值
@作者：单雁
@日期：
*************************************************************/
static int32_t K230_ParseNumber(char *token)
{
	int32_t value = 0;

	while(*token >= '0' && *token <= '9')
	{
		value = value * 10 + (*token - '0');
		token++;
	}

	return value;
}
/***********************************************************
@函数名：MouseStatusText
@入口参数：MouseStatus status,//鼠标状态枚举

功能描述：根据鼠标状态返回对应的状态文本
@作者：单雁
@日期：
*************************************************************/
static char *MouseStatusText(MouseStatus status)
{
	switch(status)
	{
		case MOUSE_STATUS_EATING: return "Eating";
		case MOUSE_STATUS_SLEEPING: return "Sleeping";
		case MOUSE_STATUS_RUNNING: return "Running";
		default: return "Unknown";
	}
}
/***********************************************************
@函数名：MouseStatusColor
@入口参数：MouseStatus status,//鼠标状态枚举

功能描述：根据鼠标状态返回对应的显示颜色
@作者：单雁
@日期：
*************************************************************/
static uint32_t MouseStatusColor(MouseStatus status)
{
	switch(status)
	{
		case MOUSE_STATUS_EATING: return LIGHT_GREEN;
		case MOUSE_STATUS_SLEEPING: return LIGHT_BLUE;
		case MOUSE_STATUS_RUNNING: return LIGHT_RED;
		default: return LIGHT_GREY;
	}
}
/***********************************************************
@函数名：MouseDisplay_Refresh
@入口参数：MouseData *data,//鼠标数据结构体指针
					 uint8_t page,//当前显示页码

功能描述：根据当前页码刷新LCD显示内容
@作者：单雁
@日期：
*************************************************************/
static void MouseDisplay_Refresh(MouseData *data, uint8_t page)
{
	LCD_SetBackColor(LCD_BLACK);
	LCD_Clear();
	LCD_ShowNumMode(Fill_Space);

	if(page == 0)
	{
		MouseDisplay_DrawSummary(data);
	}
	else
	{
		MouseDisplay_DrawMouse(data, page - 1);
	}
}
/***********************************************************
@函数名：MouseDisplay_DrawSummary
@入口参数：MouseData *data,//鼠标数据结构体指针

功能描述：在LCD上绘制鼠标数量汇总页面
@作者：单雁
@日期：
*************************************************************/
static void MouseDisplay_DrawSummary(MouseData *data)
{
	MouseDisplay_DrawHeader("Mouse Monitor", 1);
	MouseDisplay_DrawLine("Total", data->total, 55, LCD_WHITE);
	MouseDisplay_DrawLine("Eating", data->eating, 90, LIGHT_GREEN);
	MouseDisplay_DrawLine("Sleeping", data->sleeping, 125, LIGHT_BLUE);
	MouseDisplay_DrawLine("Running", data->running, 160, LIGHT_RED);

	LCD_SetAsciiFont(&ASCII_Font16);
	LCD_SetColor(LIGHT_GREY);
	LCD_DisplayString(12, 215, "KEY: next page");
}
/***********************************************************
@函数名：MouseDisplay_DrawMouse
@入口参数：MouseData *data,//鼠标数据结构体指针
					 uint8_t mouse_index,//鼠标索引

功能描述：在LCD上绘制单个鼠标的详情页面
@作者：单雁
@日期：
*************************************************************/
static void MouseDisplay_DrawMouse(MouseData *data, uint8_t mouse_index)
{
	char title[] = "Mouse 1";
	title[6] = '1' + mouse_index;

	MouseDisplay_DrawHeader(title, mouse_index + 2);
	MouseDisplay_DrawLine("Mouse ID", mouse_index + 1, 75, LCD_WHITE);
	MouseDisplay_DrawStatus(data->mouse[mouse_index], 125);

	LCD_SetAsciiFont(&ASCII_Font16);
	LCD_SetColor(LIGHT_GREY);
	LCD_DisplayString(12, 215, "KEY: next page");
}
/***********************************************************
@函数名：MouseDisplay_DrawHeader
@入口参数：char *title,//页面标题字符串
					 uint8_t page,//当前页码

功能描述：在LCD顶部绘制标题和页码信息
@作者：单雁
@日期：
*************************************************************/
static void MouseDisplay_DrawHeader(char *title, uint8_t page)
{
	LCD_SetAsciiFont(&ASCII_Font24);
	LCD_SetColor(LCD_CYAN);
	LCD_DisplayString(18, 12, title);
	LCD_SetColor(DARK_GREY);
	LCD_DrawLine_H(0, 42, 240);

	LCD_SetAsciiFont(&ASCII_Font16);
	LCD_SetColor(LIGHT_GREY);
	LCD_DisplayString(170, 18, "P");
	LCD_DisplayNumber(188, 18, page, 1);
	LCD_DisplayString(200, 18, "/4");
}
/***********************************************************
@函数名：MouseDisplay_DrawLine
@入口参数：char *label,//标签字符串
					 int32_t value,//要显示的数值
					 uint16_t y,//显示纵坐标
					 uint32_t color,//显示颜色

功能描述：在指定位置绘制一行标签和数值
@作者：单雁
@日期：
*************************************************************/
static void MouseDisplay_DrawLine(char *label, int32_t value, uint16_t y, uint32_t color)
{
	LCD_SetAsciiFont(&ASCII_Font24);
	LCD_SetColor(color);
	LCD_DisplayString(20, y, label);
	LCD_DisplayString(150, y, ":");
	LCD_DisplayNumber(175, y, value, 2);
}
/***********************************************************
@函数名：MouseDisplay_DrawStatus
@入口参数：MouseStatus status,//鼠标状态枚举
					 uint16_t y,//显示纵坐标

功能描述：在指定位置绘制鼠标状态文本
@作者：单雁
@日期：
*************************************************************/
static void MouseDisplay_DrawStatus(MouseStatus status, uint16_t y)
{
	LCD_SetAsciiFont(&ASCII_Font24);
	LCD_SetColor(LCD_WHITE);
	LCD_DisplayString(20, y, "Status:");
	LCD_SetColor(MouseStatusColor(status));
	LCD_DisplayString(20, y + 42, MouseStatusText(status));
}
