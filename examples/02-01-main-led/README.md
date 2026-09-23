# 2.1 单文件点灯

配套[第二篇博客](../../blog/02-dnesp32s3-idf-create-project-led.md)的 2.1 节。应用代码全部在 `main/main.c`；工程根目录的 `CMakeLists.txt` 和 `main/CMakeLists.txt` 是 ESP-IDF 构建所需文件。DNESP32S3 V1.2 的红色用户 LED 接 GPIO1，低电平亮。

```bash
source "$HOME/.espressif/tools/activate_idf_v5.5.5.sh" # 换成本机实际安装版本
idf.py set-target esp32s3
idf.py build
idf.py -p <实际串口> flash monitor
```

Ubuntu 26.04 和实板闪灯仍需验证。
