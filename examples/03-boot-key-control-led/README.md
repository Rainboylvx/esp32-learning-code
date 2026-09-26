# 3 BOOT 按键控制 LED

本工程用于正点原子 DNESP32S3 V1.2：板载 BOOT 按键连接 GPIO0，板载红色用户 LED 连接 GPIO1。每次经过消抖确认的按下事件会翻转一次 LED；长按不会连续触发。

```bash
# 每个新终端先激活 ESP-IDF。
source "$HOME/.espressif/tools/activate_idf_v5.5.5.sh"

# 在本工程根目录编译。
idf.py set-target esp32s3
idf.py build

# 将端口替换为插入开发板后新出现的实际串口。
idf.py -p '/dev/cu.替换为开发板端口' flash monitor
```

按 `Ctrl+]` 退出串口监视器。GPIO0 同时是启动配置引脚；上电或复位时一直按住 BOOT，芯片可能进入下载模式。
