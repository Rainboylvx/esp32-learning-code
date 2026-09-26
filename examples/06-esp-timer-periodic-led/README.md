# 06 ESP Timer 周期任务

本工程用于正点原子 DNESP32S3 V1.2。`esp_timer` 每隔 1 秒执行一次短回调，回调只给 LED 任务发送通知；LED 任务负责翻转 GPIO1 红色用户 LED 并打印实际间隔。

```bash
# 每个新终端先激活 ESP-IDF。
source "$HOME/.espressif/tools/activate_idf_v5.5.5.sh"

# 新工程首次设置目标并编译。
idf.py set-target esp32s3
idf.py build

# 将端口替换为插入开发板后新出现的实际串口。
idf.py -p '/dev/cu.替换为开发板端口' flash monitor
```

预期红色 LED 每隔 1 秒翻转一次。串口中的 `elapsed` 应接近 `1000000 us`，但软件调度会带来少量误差。按 `Ctrl+]` 退出串口监视器。
