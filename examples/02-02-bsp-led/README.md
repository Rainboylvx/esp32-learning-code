# 2.2 BSP 分层点灯

配套[第二篇博客](../../blog/02-dnesp32s3-idf-create-project-led.md)的 2.2 节。与[2.1 单文件工程](../02-01-main-led/)使用同一引脚、时序和预期现象；`main/main.c` 只描述亮灭顺序，`components/BSP/LED/` 负责 DNESP32S3 的 GPIO1 与低电平有效接线。

```bash
source "$HOME/.espressif/tools/activate_idf_v5.5.5.sh" # 换成本机实际安装版本
idf.py set-target esp32s3
idf.py build
idf.py -p <实际串口> flash monitor
```

Ubuntu 26.04 和实板闪灯仍需验证。
