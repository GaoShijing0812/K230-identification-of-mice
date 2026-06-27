<div align="center">

# 🐭 Mouse Behavior Monitor

**Real-time mouse behavior recognition system based on K230 + STM32F407**

[![Platform](https://img.shields.io/badge/Platform-K230%20%7C%20STM32F407-blue)]()
[![Model](https://img.shields.io/badge/AI%20Model-YOLO11-green)]()
[![IDE](https://img.shields.io/badge/IDE-Keil%20uVision-orange)]()
[![License](https://img.shields.io/badge/License-Learning-lightgrey)]()

**[English](#english) · [中文](#中文)**

</div>

---

<a name="english"></a>

# English

## Table of Contents

- [Introduction](#introduction)
- [Demo](#demo)
- [Highlights](#highlights)
- [Architecture](#architecture)
- [Getting Started](#getting-started)
- [Wiring](#wiring)
- [Communication Protocol](#communication-protocol)
- [Project Structure](#project-structure)
- [STM32 Firmware Workflow](#stm32-firmware-workflow)
- [Display Pages](#display-pages)
- [Model Details](#model-details)
- [Tech Stack](#tech-stack)
- [License](#license)

## Introduction

An embedded AI system that uses YOLO11 object detection to classify mouse behaviors (eating, sleeping, running) in real-time via camera, with results displayed on an STM32-driven LCD screen.

The K230 board runs the YOLO11 model for video inference, then sends detection results to the STM32F407 via UART. The STM32 parses the data and renders a multi-page status display on a 240×240 ST7789 LCD.

## Demo

<!-- TODO: Add demo video or screenshot here -->

📸 *Screenshots / demo GIF to be added*

## Highlights

- 🎯 **Real-time Detection** — YOLO11 on K230 with 320×320 input, 0.7 confidence threshold
- 🔌 **Dual-chip Architecture** — K230 handles AI inference; STM32 handles display
- 📺 **Multi-page LCD UI** — Summary page + 3 individual mouse detail pages, switchable via button
- 📡 **Lightweight UART Protocol** — CSV-formatted frames at 115200 baud

## Architecture

```
Camera ──▶ K230 (YOLO11 Inference) ──UART──▶ STM32F407 ──SPI──▶ ST7789 LCD
                                                      ◀──Button──┘ (Page Switch)
```

## Getting Started

### Prerequisites

| Component | Requirement |
|-----------|-------------|
| AI Board | 01Studio CanMV K230 |
| MCU Board | STM32F407ZGT6 Development Board |
| Display | ST7789 240×240 SPI LCD |
| Camera | K230 onboard or external camera |
| Programmer | J-Link / ST-Link |
| IDE | Keil μVision 5 |

### K230 Setup

1. Copy `main.py` to the K230 SD card root directory
2. Download `yolo11n_det_320.kmodel` from [01Studio AI Platform](https://ai.01studio.cc) and copy to SD card
3. Wire UART (see [Wiring](#wiring) section)
4. Power on — the system starts detection automatically

### STM32 Setup

1. Open `STM32F407ZGT6/Project/STM32F407.uvprojx` in Keil μVision
2. Build the project
3. Flash to the STM32F407ZGT6 board via J-Link/ST-Link
4. Wire the LCD and UART
5. Power on — the LCD displays the monitoring interface

### Custom Model Training

To train a custom behavior detection model, visit the [01Studio AI Platform](https://ai.01studio.cc):

1. Collect and annotate mouse behavior image dataset
2. Train a YOLO11 model on the platform
3. Export `.kmodel` and replace the file on the SD card

## Wiring

### UART (K230 ↔ STM32)

| K230 | STM32 | Function |
|------|-------|----------|
| TX (IO5) | PA10 (RX) | K230 → STM32 |
| RX (IO6) | PA9 (TX) | STM32 → K230 |
| GND | GND | Common Ground |

### SPI LCD (STM32 → ST7789)

| STM32 | LCD | Function |
|-------|-----|----------|
| PB3 | SCK | SPI Clock |
| PB5 | SDA | SPI Data |
| PD11 | CS | Chip Select |
| PD12 | DC | Data/Command |
| PD13 | BLK | Backlight |

## Communication Protocol

K230 sends data frames over UART at **115200 baud** in the following format:

```
total,eating,sleeping,running,mouse1,mouse2,mouse3\n
```

| Code | Status |
|------|--------|
| `E` | Eating |
| `S` | Sleeping |
| `R` | Running |
| `U` | Unknown |

**Example:**
```
3,1,1,1,E,S,R    → 3 mice: 1 eating, 1 sleeping, 1 running
2,0,0,2,R,R,U    → 2 mice: both running, slot 3 empty
```

## Project Structure

```
K230-identification-of-mice/
├── main.py                         # K230 MicroPython — YOLO11 inference + UART
└── STM32F407ZGT6/
    ├── MY/                         # Application layer
    │   ├── main.c                  # Entry point: data parsing + LCD rendering
    │   └── stm32f4xx_it.c          # Interrupt handlers
    ├── Drive/                      # Hardware drivers
    │   ├── Source/                  # Driver implementations
    │   └── Include/                # Driver headers
    │       ├── usart.h             #   UART (USART1, 115200bps)
    │       ├── lcd_spi_154.h       #   ST7789 LCD (SPI3)
    │       ├── led.h               #   LED
    │       └── key.h               #   Button input
    ├── STM32/                      # STM32 Standard Peripheral Library
    │   ├── CMSIS/
    │   └── STM32F4xx_StdPeriph_Driver/
    └── Project/                    # Keil μVision project files
        └── STM32F407.uvprojx
```

## STM32 Firmware Workflow

Below is the complete program flow of the STM32F407 firmware, from startup to steady-state operation.

### Initialization

When the MCU powers on, `main()` calls the following initialization functions in sequence:

```
main()
  ├── Delay_Init()        // Configure SysTick for 1ms interrupt
  ├── LED_Init()          // Configure PC13 as LED output
  ├── KEY_Init()          // Configure PA15 as button input with pull-up
  ├── Usart_Config()      // Configure USART1: PA9(TX)/PA10(RX), 115200bps, RX interrupt
  └── SPI_LCD_Init()      // Full LCD initialization
      ├── LCD_GPIO_Config()     // Configure SPI3 pins: SCK(PB3), SDA(PB5), CS(PD11), DC(PD12), Backlight(PD13)
      ├── LCD_SPI_Config()      // Configure SPI3 as master, 8-bit, 21MHz
      ├── ST7789 init commands  // Send voltage/gamma/display commands to ST7789
      ├── LCD_SetDirection(V)   // Set vertical display, 240×240
      ├── LCD_Clear()           // Fill screen with black
      └── LCD_Backlight_ON      // Turn on backlight
```

### Main Loop

After initialization, the program enters the main `while(1)` loop. Each iteration performs three checks:

```
while(1)
  │
  ├── ① K230_ReadLine() → Check if a complete UART data line is ready
  │       ├── YES → K230_ParseMouseData() → Parse and update g_mouse_data
  │       │         → Set g_lcd_need_refresh = 1
  │       └── NO  → Skip
  │
  ├── ② KEY_Scan() → Check if the button was pressed
  │       ├── KEY_ON → g_current_page++ (cycle 0→1→2→3→0)
  │       │         → Set g_lcd_need_refresh = 1
  │       └── KEY_OFF → Skip
  │
  └── ③ if(g_lcd_need_refresh) → Redraw the LCD
          ├── MouseDisplay_Refresh(&g_mouse_data, g_current_page)
          └── g_lcd_need_refresh = 0
```

### UART Receive Flow

USART1 receives data via interrupt. Each byte is processed by `K230_UartRxHandler()` (in `stm32f4xx_it.c`):

```
USART1_IRQHandler (interrupt)
  └── K230_UartRxHandler()
        ├── Read 1 byte from USART1 data register
        ├── If '\r' → Ignore, return
        ├── If '\n' → End of line: copy buffer to k230_ready_line,
        │              set k230_line_ready = 1, reset index
        └── Otherwise → Store byte in k230_line_buffer[k230_line_index++]
```

`K230_ReadLine()` checks the `k230_line_ready` flag; when set, it copies the ready line to the caller's buffer and resets the flag (with interrupts disabled for atomicity).

### Data Parsing Flow

`K230_ParseMouseData()` in `main.c` parses the CSV line step by step:

```
K230_ParseMouseData(line, &g_mouse_data)
  │
  ├── K230_NextToken() → Extract "total"         → K230_ParseNumber() → temp.total
  ├── K230_NextToken() → Extract "eating"         → K230_ParseNumber() → temp.eating
  ├── K230_NextToken() → Extract "sleeping"       → K230_ParseNumber() → temp.sleeping
  ├── K230_NextToken() → Extract "running"        → K230_ParseNumber() → temp.running
  │
  ├── K230_NextToken() + K230_ParseStatus() → Mouse #1 status
  │     ├── 'E'/'e' → MOUSE_STATUS_EATING
  │     ├── 'S'/'s' → MOUSE_STATUS_SLEEPING
  │     ├── 'R'/'r' → MOUSE_STATUS_RUNNING
  │     └── 'U'/'u' → MOUSE_STATUS_UNKNOWN
  │
  ├── ... same for Mouse #2 and #3
  │
  └── Clamp all values to [0, MOUSE_MAX_COUNT] → Write to g_mouse_data
```

### Display Refresh Flow

`MouseDisplay_Refresh()` clears the screen and draws the current page:

```
MouseDisplay_Refresh(data, page)
  │
  ├── LCD_Clear()  // Clear screen with black background
  │
  ├── if page == 0 → MouseDisplay_DrawSummary(data)
  │     ├── DrawHeader("Mouse Monitor", P1/4)
  │     ├── DrawLine("Total",    total,    y=55)   // White
  │     ├── DrawLine("Eating",   eating,   y=90)   // Green
  │     ├── DrawLine("Sleeping", sleeping, y=125)  // Blue
  │     ├── DrawLine("Running",  running,  y=160)  // Red
  │     └── Display "KEY: next page" at bottom
  │
  └── if page == 1~3 → MouseDisplay_DrawMouse(data, page-1)
        ├── DrawHeader("Mouse N", P(N+1)/4)
        ├── DrawLine("Mouse ID", N+1, y=75)
        ├── DrawStatus(status, y=125)
        │     ├── Eating   → Green
        │     ├── Sleeping → Blue
        │     ├── Running  → Red
        │     └── Unknown  → Grey
        └── Display "KEY: next page" at bottom
```

### Function Reference

| File | Function | Description |
|---|---|---|
| `MY/main.c` | `main()` | Entry point: init + main loop |
| `MY/main.c` | `K230_ParseMouseData()` | Parse CSV line into MouseData struct |
| `MY/main.c` | `K230_ParseStatus()` | Convert char (E/S/R/U) to MouseStatus enum |
| `MY/main.c` | `K230_NextToken()` | Extract next comma-separated token |
| `MY/main.c` | `K230_ParseNumber()` | Convert digit string to int32 |
| `MY/main.c` | `MouseDisplay_Refresh()` | Clear screen and draw current page |
| `MY/main.c` | `MouseDisplay_DrawSummary()` | Draw summary page (P1/4) |
| `MY/main.c` | `MouseDisplay_DrawMouse()` | Draw single mouse page (P2~P4) |
| `MY/main.c` | `MouseDisplay_DrawHeader()` | Draw title bar with page number |
| `MY/main.c` | `MouseDisplay_DrawLine()` | Draw a label + number row |
| `MY/main.c` | `MouseDisplay_DrawStatus()` | Draw status text with color |
| `MY/main.c` | `MouseStatusText()` | Return status string |
| `MY/main.c` | `MouseStatusColor()` | Return status color |
| `Drive/usart.c` | `Usart_Config()` | Init USART1 (115200bps, RX interrupt) |
| `Drive/usart.c` | `K230_UartRxHandler()` | ISR: buffer incoming bytes, detect line end |
| `Drive/usart.c` | `K230_ReadLine()` | Read one complete line from buffer |
| `Drive/key.c` | `KEY_Init()` | Configure button GPIO (PA15, pull-up) |
| `Drive/key.c` | `KEY_Scan()` | Detect button press with debounce |
| `Drive/led.c` | `LED_Init()` | Configure LED GPIO (PC13, push-pull) |
| `Drive/delay.c` | `Delay_Init()` | Configure SysTick for 1ms ticks |
| `Drive/delay.c` | `Delay_ms()` | Blocking delay in milliseconds |
| `Drive/lcd_spi_154.c` | `SPI_LCD_Init()` | Full ST7789 LCD init |
| `Drive/lcd_spi_154.c` | `LCD_Clear()` | Fill screen with background color |
| `Drive/lcd_spi_154.c` | `LCD_SetDirection()` | Set display orientation |
| `Drive/lcd_spi_154.c` | `LCD_SetColor()` | Set foreground drawing color |
| `Drive/lcd_spi_154.c` | `LCD_SetBackColor()` | Set background color |
| `Drive/lcd_spi_154.c` | `LCD_SetAsciiFont()` | Set ASCII font size |
| `Drive/lcd_spi_154.c` | `LCD_DisplayString()` | Draw ASCII string at (x,y) |
| `Drive/lcd_spi_154.c` | `LCD_DisplayNumber()` | Draw formatted number at (x,y) |
| `Drive/lcd_spi_154.c` | `LCD_DrawLine_H()` | Draw horizontal line |

## Display Pages

The LCD cycles through 4 pages via the onboard button:

| Page | Content |
|------|---------|
| **P1/4** | Summary — Total / Eating / Sleeping / Running counts |
| **P2/4** | Mouse #1 — ID + current status |
| **P3/4** | Mouse #2 — ID + current status |
| **P4/4** | Mouse #3 — ID + current status |

## Model Details

| Parameter | Value |
|-----------|-------|
| Model | YOLO11n Detection |
| File | `yolo11n_det_320.kmodel` |
| Input Size | 320 × 320 |
| Confidence Threshold | 0.7 |
| NMS Threshold | 0.45 |
| Classes | `Eating Mouse`, `Sleeping Mouse`, `Normal Mouse` |
| Training Platform | [01Studio AI](https://ai.01studio.cc) |

## Tech Stack

| Component | Technology |
|-----------|-----------|
| K230 Firmware | MicroPython (CanMV) |
| STM32 Firmware | C / STM32F4xx Standard Peripheral Library |
| IDE | Keil μVision 5 |
| AI Training | [01Studio Online Platform](https://ai.01studio.cc) |

## License

This project is for educational and learning purposes only.

---

<a name="中文"></a>

# 中文

## 目录

- [项目简介](#项目简介)
- [演示](#演示)
- [功能亮点](#功能亮点)
- [系统架构](#系统架构)
- [快速开始](#快速开始)
- [接线说明](#接线说明)
- [通信协议](#通信协议)
- [项目结构](#项目结构)
- [STM32 固件工作流程](#stm32-固件工作流程)
- [显示页面](#显示页面)
- [模型信息](#模型信息)
- [技术栈](#技术栈)
- [许可证](#许可证)

## 项目简介

基于 YOLO11 目标检测的嵌入式 AI 系统，通过摄像头实时识别小鼠的进食、睡觉、正常活动三种行为状态，并在 STM32 驱动的 LCD 屏幕上显示监测结果。

K230 负责 YOLO11 模型推理，通过 UART 将检测结果发送至 STM32F407，STM32 解析数据并在 240×240 ST7789 LCD 上渲染多页状态显示。

## 演示

<!-- TODO: 添加演示视频或截图 -->

📸 *截图 / 演示 GIF 待添加*

## 功能亮点

- 🎯 **实时检测** — K230 上运行 YOLO11，320×320 输入，0.7 置信度阈值
- 🔌 **双芯片架构** — K230 负责 AI 推理，STM32 负责显示控制
- 📺 **LCD 多页显示** — 汇总页 + 3 个独立小鼠详情页，按键切换
- 📡 **轻量通信协议** — CSV 格式数据帧，115200 波特率

## 系统架构

```
摄像头 ──▶ K230（YOLO11 推理）──UART──▶ STM32F407 ──SPI──▶ ST7789 LCD
                                                    ◀──按键──┘（翻页）
```

## 快速开始

### 硬件清单

| 硬件 | 型号 |
|------|------|
| AI 视觉板 | 01Studio CanMV K230（庐山派） |
| 主控板 | STM32F407ZGT6 开发板 |
| 显示屏 | ST7789 240×240 SPI LCD |
| 摄像头 | K230 板载或外接摄像头 |
| 调试器 | J-Link / ST-Link |
| IDE | Keil μVision 5 |

### K230 部署

1. 将 `main.py` 拷贝至 K230 SD 卡根目录
2. 从 [01Studio AI 平台](https://ai.01studio.cc) 下载 `yolo11n_det_320.kmodel` 模型文件，拷贝至 SD 卡
3. 连接 UART 接线（见 [接线说明](#接线说明)）
4. 上电运行，系统自动启动检测

### STM32 部署

1. 使用 Keil μVision 打开 `STM32F407ZGT6/Project/STM32F407.uvprojx`
2. 编译工程
3. 通过 J-Link / ST-Link 烧录至 STM32F407ZGT6 开发板
4. 连接 LCD 屏幕和 UART 接线
5. 上电后 LCD 显示监测界面

### 自定义模型训练

如需训练自定义模型，访问 [01Studio AI 平台](https://ai.01studio.cc)：

1. 采集并标注小鼠行为图像数据集
2. 在平台上训练 YOLO11 模型
3. 导出 `.kmodel` 文件并替换 SD 卡中的模型

## 接线说明

### UART（K230 ↔ STM32）

| K230 | STM32 | 功能 |
|------|-------|------|
| TX (IO5) | PA10 (RX) | K230 发送 → STM32 接收 |
| RX (IO6) | PA9 (TX) | STM32 发送 → K230 接收 |
| GND | GND | 共地 |

### SPI LCD（STM32 → ST7789）

| STM32 | LCD | 功能 |
|-------|-----|------|
| PB3 | SCK | SPI 时钟 |
| PB5 | SDA | SPI 数据 |
| PD11 | CS | 片选 |
| PD12 | DC | 数据/命令选择 |
| PD13 | BLK | 背光 |

## 通信协议

K230 通过 UART 以 **115200 波特率**发送以下格式的数据帧：

```
总数,吃饭数,睡觉数,跑步数,鼠1状态,鼠2状态,鼠3状态\n
```

| 编码 | 状态 |
|------|------|
| `E` | 进食（Eating） |
| `S` | 睡觉（Sleeping） |
| `R` | 正常活动（Running） |
| `U` | 未知（Unknown） |

**示例：**
```
3,1,1,1,E,S,R    → 3只小鼠：1只进食、1只睡觉、1只活动
2,0,0,2,R,R,U    → 2只小鼠：2只活动，第3位无数据
```

## 项目结构

```
K230-identification-of-mice/
├── main.py                         # K230 MicroPython — YOLO11 推理 + UART 输出
└── STM32F407ZGT6/
    ├── MY/                         # 应用层
    │   ├── main.c                  # 程序入口：数据解析 + LCD 显示逻辑
    │   └── stm32f4xx_it.c          # 中断服务函数
    ├── Drive/                      # 硬件驱动
    │   ├── Source/                  # 驱动实现
    │   └── Include/                # 驱动头文件
    │       ├── usart.h             #   UART 驱动（USART1，115200bps）
    │       ├── lcd_spi_154.h       #   ST7789 LCD 驱动（SPI3）
    │       ├── led.h               #   LED 驱动
    │       └── key.h               #   按键输入驱动
    ├── STM32/                      # STM32 标准外设库
    │   ├── CMSIS/
    │   └── STM32F4xx_StdPeriph_Driver/
    └── Project/                    # Keil μVision 工程文件
        └── STM32F407.uvprojx
```

## STM32 固件工作流程

以下是 STM32F407 固件的完整工作流程，从上电启动到稳态运行。

### 系统初始化

上电后 `main()` 依次调用以下初始化函数：

```
main()
  ├── Delay_Init()        // 配置 SysTick 1ms 中断，用于毫秒延时
  ├── LED_Init()          // 配置 PC13 为 LED 推挽输出
  ├── KEY_Init()          // 配置 PA15 为按键输入，上拉模式
  ├── Usart_Config()      // 配置 USART1：PA9(TX)/PA10(RX)，115200 波特率，开启接收中断
  └── SPI_LCD_Init()      // LCD 完整初始化
      ├── LCD_GPIO_Config()     // 配置 SPI3 引脚：SCK(PB3), SDA(PB5), CS(PD11), DC(PD12), 背光(PD13)
      ├── LCD_SPI_Config()      // 配置 SPI3 为主模式，8 位数据，21MHz 时钟
      ├── ST7789 初始化指令      // 发送电压/伽马/显示等初始化指令
      ├── LCD_SetDirection(V)   // 设置竖屏显示，240×240
      ├── LCD_Clear()           // 黑色清屏
      └── LCD_Backlight_ON      // 打开背光
```

### 主循环

初始化完成后进入 `while(1)` 主循环，每次循环执行三个检查：

```
while(1)
  │
  ├── ① K230_ReadLine() → 检查是否有完整的 UART 数据行到达
  │       ├── 有 → K230_ParseMouseData() → 解析并更新小鼠数据结构体
  │       │       → 置 g_lcd_need_refresh = 1（标记需要刷新显示）
  │       └── 无 → 跳过
  │
  ├── ② KEY_Scan() → 检测按键是否按下
  │       ├── 按下 → g_current_page++（翻页：0→1→2→3→0 循环）
  │       │        → 置 g_lcd_need_refresh = 1
  │       └── 未按下 → 跳过
  │
  └── ③ if(g_lcd_need_refresh) → 如果需要刷新，重绘 LCD
          ├── MouseDisplay_Refresh(&g_mouse_data, g_current_page)
          └── g_lcd_need_refresh = 0
```

### UART 接收流程

USART1 通过中断接收数据，每个字节由 `K230_UartRxHandler()` 处理（位于 `stm32f4xx_it.c`）：

```
USART1_IRQHandler（中断入口）
  └── K230_UartRxHandler()
        ├── 从 USART1 数据寄存器读取 1 字节
        ├── 如果是 '\r' → 忽略，返回
        ├── 如果是 '\n' → 行结束：将缓冲区复制到 k230_ready_line，
        │                  置 k230_line_ready = 1，重置索引
        └── 其他字符 → 存入 k230_line_buffer[k230_line_index++]
```

`K230_ReadLine()` 检查 `k230_line_ready` 标志；当标志置位时，将就绪行复制到调用方缓冲区并重置标志（关中断保证原子性）。

### 数据解析流程

`main.c` 中的 `K230_ParseMouseData()` 逐步解析 CSV 数据行：

```
K230_ParseMouseData(line, &g_mouse_data)
  │
  ├── K230_NextToken() → 提取"总数"字段       → K230_ParseNumber() → temp.total
  ├── K230_NextToken() → 提取"进食数"字段     → K230_ParseNumber() → temp.eating
  ├── K230_NextToken() → 提取"睡觉数"字段     → K230_ParseNumber() → temp.sleeping
  ├── K230_NextToken() → 提取"活动数"字段     → K230_ParseNumber() → temp.running
  │
  ├── K230_NextToken() + K230_ParseStatus() → 小鼠 1 状态
  │     ├── 'E'/'e' → MOUSE_STATUS_EATING（进食）
  │     ├── 'S'/'s' → MOUSE_STATUS_SLEEPING（睡觉）
  │     ├── 'R'/'r' → MOUSE_STATUS_RUNNING（活动）
  │     └── 'U'/'u' → MOUSE_STATUS_UNKNOWN（未知）
  │
  ├── ... 小鼠 2、3 同理
  │
  └── 将所有数值限制在 [0, 3] 范围内 → 写入全局数据结构 g_mouse_data
```

### 显示刷新流程

`MouseDisplay_Refresh()` 清屏并绘制当前页面：

```
MouseDisplay_Refresh(data, page)
  │
  ├── LCD_Clear()  // 黑色背景清屏
  │
  ├── page == 0 → MouseDisplay_DrawSummary(data)  // 绘制汇总页
  │     ├── DrawHeader("Mouse Monitor", P1/4)      // 标题 + 页码
  │     ├── DrawLine("Total",    total,    y=55)    // 白色
  │     ├── DrawLine("Eating",   eating,   y=90)    // 绿色
  │     ├── DrawLine("Sleeping", sleeping, y=125)   // 蓝色
  │     ├── DrawLine("Running",  running,  y=160)   // 红色
  │     └── 底部显示 "KEY: next page"
  │
  └── page == 1~3 → MouseDisplay_DrawMouse(data, page-1)  // 绘制单只小鼠详情页
        ├── DrawHeader("Mouse N", P(N+1)/4)               // 标题
        ├── DrawLine("Mouse ID", N+1, y=75)               // 小鼠编号
        ├── DrawStatus(status, y=125)                      // 带颜色的状态文字
        │     ├── Eating   → 绿色
        │     ├── Sleeping → 蓝色
        │     ├── Running  → 红色
        │     └── Unknown  → 灰色
        └── 底部显示 "KEY: next page"
```

### 函数速查表

| 文件 | 函数 | 功能 |
|---|---|---|
| `MY/main.c` | `main()` | 程序入口：初始化 + 主循环 |
| `MY/main.c` | `K230_ParseMouseData()` | 解析 CSV 行为小鼠数据结构体 |
| `MY/main.c` | `K230_ParseStatus()` | 将字符（E/S/R/U）转为状态枚举 |
| `MY/main.c` | `K230_NextToken()` | 提取下一个逗号分隔字段 |
| `MY/main.c` | `K230_ParseNumber()` | 数字字符串转整数 |
| `MY/main.c` | `MouseDisplay_Refresh()` | 清屏并绘制当前页面 |
| `MY/main.c` | `MouseDisplay_DrawSummary()` | 绘制汇总页面（P1/4） |
| `MY/main.c` | `MouseDisplay_DrawMouse()` | 绘制单只小鼠详情页（P2~P4） |
| `MY/main.c` | `MouseDisplay_DrawHeader()` | 绘制标题栏和页码 |
| `MY/main.c` | `MouseDisplay_DrawLine()` | 绘制一行标签 + 数值 |
| `MY/main.c` | `MouseDisplay_DrawStatus()` | 绘制带颜色的状态文字 |
| `MY/main.c` | `MouseStatusText()` | 返回状态文本 |
| `MY/main.c` | `MouseStatusColor()` | 返回状态对应颜色 |
| `Drive/usart.c` | `Usart_Config()` | 初始化 USART1（115200bps，开接收中断） |
| `Drive/usart.c` | `K230_UartRxHandler()` | 中断服务：缓存字节，检测行尾 |
| `Drive/usart.c` | `K230_ReadLine()` | 从缓冲区读取一行完整数据 |
| `Drive/key.c` | `KEY_Init()` | 初始化按键 GPIO（PA15，上拉） |
| `Drive/key.c` | `KEY_Scan()` | 带消抖的按键扫描 |
| `Drive/led.c` | `LED_Init()` | 初始化 LED GPIO（PC13，推挽输出） |
| `Drive/delay.c` | `Delay_Init()` | 配置 SysTick 1ms 时基 |
| `Drive/delay.c` | `Delay_ms()` | 毫秒级阻塞延时 |
| `Drive/lcd_spi_154.c` | `SPI_LCD_Init()` | LCD 完整初始化 |
| `Drive/lcd_spi_154.c` | `LCD_Clear()` | 用背景色填充全屏 |
| `Drive/lcd_spi_154.c` | `LCD_SetDirection()` | 设置显示方向 |
| `Drive/lcd_spi_154.c` | `LCD_SetColor()` | 设置前景色 |
| `Drive/lcd_spi_154.c` | `LCD_SetBackColor()` | 设置背景色 |
| `Drive/lcd_spi_154.c` | `LCD_SetAsciiFont()` | 设置 ASCII 字体大小 |
| `Drive/lcd_spi_154.c` | `LCD_DisplayString()` | 在指定位置显示字符串 |
| `Drive/lcd_spi_154.c` | `LCD_DisplayNumber()` | 在指定位置显示数字 |
| `Drive/lcd_spi_154.c` | `LCD_DrawLine_H()` | 画水平线 |

## 显示页面

通过按键在 4 个页面之间切换：

| 页面 | 内容 |
|------|------|
| **P1/4** | 汇总页 — 总数 / 进食数 / 睡觉数 / 活动数 |
| **P2/4** | 小鼠 1 详情 — 编号 + 当前状态 |
| **P3/4** | 小鼠 2 详情 — 编号 + 当前状态 |
| **P4/4** | 小鼠 3 详情 — 编号 + 当前状态 |

## 模型信息

| 参数 | 值 |
|------|------|
| 模型 | YOLO11n Detection |
| 文件 | `yolo11n_det_320.kmodel` |
| 输入尺寸 | 320 × 320 |
| 置信度阈值 | 0.7 |
| NMS 阈值 | 0.45 |
| 类别 | `吃东西的小鼠`、`睡觉的小鼠`、`正常小鼠` |
| 训练平台 | [01Studio AI 平台](https://ai.01studio.cc) |

## 技术栈

| 组件 | 技术 |
|------|------|
| K230 固件 | MicroPython (CanMV) |
| STM32 固件 | C / STM32F4xx 标准外设库 |
| IDE | Keil μVision 5 |
| AI 训练 | [01Studio 在线训练平台](https://ai.01studio.cc) |

## 许可证

本项目仅供学习交流使用。
