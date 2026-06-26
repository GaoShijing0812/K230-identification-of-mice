"""
实验名称：在线训练-YOLO图像检测: 基于摄像头 + UART通信
实验平台：01Studio CanMV K230/CanMV K230 mini（庐山派）
说明：可以通过display="xxx"参数选择"hdmi"、"lcd3_5"(3.5寸mipi屏)或"lcd2_4"(2.4寸mipi屏)显示方式
新增功能：
  - 通过UART与STM32F103C8T6通信
  - 识别到正常小鼠 -> 发送 "Normal"
  - 识别到吃东西的小鼠 -> 发送 "Eatting"
  - 识别到睡觉的小鼠 -> 发送 "Sleep"
  - 结束时发送统计计数
01科技（01Studio）在线训练平台：https://ai.01studio.cc
"""

from libs.PipeLine import PipeLine
from libs.YOLO import YOLO11
from libs.Utils import *
from media.sensor import *
import os, sys, gc
import ulab.numpy as np
import image
import time
from machine import UART, FPIOA

# 这里为自动生成内容，自定义场景请修改为您自己的模型路径、标签名称、模型输入大小
kmodel_path="/sdcard/yolo11n_det_320.kmodel"
labels = {0: '吃东西的小鼠', 1: '睡觉的小鼠', 2: '正常小鼠'}
model_input_size = [320, 320]
# 显示模式，可以选择"hdmi"、"lcd3_5"(3.5寸mipi屏)和"lcd2_4"(2.4寸mipi屏)
display = "lcd3_5"

if display == "hdmi":
    display_mode = "hdmi"
    display_size = [1920, 1080]

elif display == "lcd3_5":
    display_mode = "st7701"
    display_size = [800, 480]

elif display == "lcd2_4":
    display_mode = "st7701"
    display_size = [640, 480]

rgb888p_size = [640, 360]

# 初始化PipeLine
pl = PipeLine(
    rgb888p_size=rgb888p_size, display_size=display_size, display_mode=display_mode
)

if display == "lcd2_4":
    pl.create(sensor=Sensor(width=1280, height=960))  # 创建PipeLine实例，画面4:3

else:
    pl.create(sensor=Sensor(width=1920, height=1080))  # 创建PipeLine实例

display_size = pl.get_display_size()

# 初始化YOLO11实例
confidence_threshold = 0.7  # 置信度（提高以减少误检）
nms_threshold = 0.45
yolo = YOLO11(
    task_type="detect",
    mode="video",
    kmodel_path=kmodel_path,
    labels=labels,
    rgb888p_size=rgb888p_size,
    model_input_size=model_input_size,
    display_size=display_size,
    conf_thresh=confidence_threshold,
    nms_thresh=nms_threshold,
    max_boxes_num=50,
    debug_mode=0,
)
yolo.config_preprocess()

# ==================== UART 初始化（与STM32F103C8T6通信）====================
# 庐山派 K230 必须先用 FPIOA 配置引脚功能，再初始化 UART
# 接线说明：
#   K230 TX引脚 -> STM32 RX (PA10)
#   K230 RX引脚 -> STM32 TX (PA9)
#   K230 GND     -> STM32 GND
# 如果使用的引脚不同，请修改下方 PIN_UART2_TX / PIN_UART2_RX 的值

fpioa = FPIOA()

# 引脚号请根据实际接线修改（以下为示例值）
PIN_UART2_TX = 5   # K230 的 TX 引脚号
PIN_UART2_RX = 6   # K230 的 RX 引脚号

fpioa.set_function(PIN_UART2_TX, FPIOA.UART2_TXD)
fpioa.set_function(PIN_UART2_RX, FPIOA.UART2_RXD)

uart = UART(2, baudrate=115200, bits=UART.EIGHTBITS,
            parity=UART.PARITY_NONE, stop=UART.STOPBITS_ONE)
print("UART2 初始化完成，TX=IO{}, RX=IO{}, 波特率: 115200".format(
    PIN_UART2_TX, PIN_UART2_RX))

# ==================== 计数器 ====================
count_normal = 0   # 正常小鼠计数
count_eating = 0   # 吃东西小鼠计数
count_sleep  = 0   # 睡觉小鼠计数

clock = time.clock()

print("=" * 40)
print("小鼠行为监测系统启动")
print("按 Ctrl+C 停止监测并发送统计结果")
print("=" * 40)

try:
    while True:

        clock.tick()

        # 逐帧推理
        img = pl.get_frame()
        res = yolo.run(img)
        yolo.draw_result(res, pl.osd_img)
        print(res)  # 打印识别结果

        # ==================== 处理检测结果 & UART发送 ====================
        mouse_statuses = []  # 当前帧每只小鼠的状态码
        frame_eating = 0     # 本帧吃饭数量
        frame_sleep  = 0     # 本帧睡觉数量
        frame_normal = 0     # 本帧跑步数量

        if res:
            # res格式: [[bboxes], [class_ids], [confidences]]
            class_ids = res[1]   # 类别列表: [2]
            confidences = res[2] # 置信度列表: [0.73]
            det_count = len(class_ids)  # 检测到的目标数量
            print("[DEBUG] 检测到{}个目标, 类别={}, 置信度={}".format(
                det_count, class_ids, confidences))

            for i in range(det_count):
                class_id = int(class_ids[i])
                conf = float(confidences[i])

                if conf < confidence_threshold:
                    continue  # 置信度不够，跳过

                if class_id == 0:        # 吃东西的小鼠
                    mouse_statuses.append("E")
                    frame_eating += 1
                    count_eating += 1
                elif class_id == 1:      # 睡觉的小鼠
                    mouse_statuses.append("S")
                    frame_sleep += 1
                    count_sleep += 1
                elif class_id == 2:      # 正常小鼠（跑步）
                    mouse_statuses.append("R")
                    frame_normal += 1
                    count_normal += 1

        # 按协议格式发送: 总数,吃饭数,睡觉数,跑步数,鼠1状态,鼠2状态,鼠3状态
        total = len(mouse_statuses)
        if total > 0:
            while len(mouse_statuses) < 3:
                mouse_statuses.append("U")
            msg = "{},{},{},{},{},{},{}\n".format(
                total, frame_eating, frame_sleep, frame_normal,
                mouse_statuses[0], mouse_statuses[1], mouse_statuses[2]
            )
            uart.write(msg)
            print("[UART] 发送: " + msg.strip())

        pl.show_image()
        gc.collect()

        print("FPS:", clock.fps())  # 打印帧率

except KeyboardInterrupt:
    # Ctrl+C 停止
    print("\n监测手动停止...")

finally:
    # ==================== 发送统计结果 ====================
    print("\n" + "=" * 40)
    print("监测结束，发送统计数据...")

    print("累计统计 - Normal: {}, Eatting: {}, Sleep: {}".format(
        count_normal, count_eating, count_sleep
    ))
    print("=" * 40)

    # 释放资源
    yolo.deinit()
    pl.destroy()
