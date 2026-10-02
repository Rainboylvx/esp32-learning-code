# 04 GPIO 外部中断控制 LED

本工程用于正点原子 DNESP32S3 V1.2：BOOT 按键连接 GPIO0，红色用户 LED 连接 GPIO1。GPIO0 的下降沿由 ISR 捕获，ISR 只把引脚号送入 FreeRTOS 队列；普通任务负责消抖、等待松开和翻转 LED。

```bash
# 每个新终端先激活 ESP-IDF。
source "$HOME/.espressif/tools/activate_idf_v5.5.5.sh"

# 在本工程根目录首次设置目标并编译。
idf.py set-target esp32s3
idf.py build

# 将端口替换为插入开发板后新出现的实际串口。
idf.py -p '/dev/cu.替换为开发板端口' flash monitor
```

每次确认按下 BOOT，LED 翻转一次；长按不会连续翻转。串口会输出 `BOOT press accepted`。按 `Ctrl+]` 退出串口监视器。GPIO0 同时是启动配置引脚；上电或复位时一直按住 BOOT，芯片可能进入下载模式。
