# 05 FreeRTOS 任务、队列与任务通知

本工程用于正点原子 DNESP32S3 V1.2，通过板载 BOOT 按键和红色 LED 演示三种 FreeRTOS 基础能力：

- `key_task` 轮询 GPIO0，把带数据的按键事件写入队列；
- `led_task` 阻塞等待队列，收到事件后翻转 GPIO1 红色 LED；
- `status_task` 阻塞等待任务通知，统计已经处理的事件数。

```bash
# 每个新终端先激活 ESP-IDF。
source "$HOME/.espressif/tools/activate_idf_v5.5.5.sh"

# 在本工程根目录首次设置目标并编译。
idf.py set-target esp32s3
idf.py build

# 将端口替换为插入开发板后新出现的实际串口。
idf.py -p '/dev/cu.替换为开发板端口' flash monitor
```

按 `Ctrl+]` 退出串口监视器。GPIO0 同时是启动配置引脚；上电或复位时一直按住 BOOT，芯片可能进入下载模式。
