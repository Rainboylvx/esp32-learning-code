# 资料：ESP-IDF 入门工程合集

本目录是从 `y9000x` 上收集整理的 **ESP-IDF（非 Arduino）** 示例工程，主要用于查阅、对照和写成博客素材。

收录范围只有两类：

1. **点灯（GPIO / LED 入门）** —— 从空工程到 WS2812 灯带，共 5 个不同来源的写法
2. **BLE 遥控小车** —— 一个完整的外设综合工程（电机 + BLE + LED 状态灯）

> 工程里的 `build/`、`managed_components/`、嵌套 `.git/` 已剔除，其余文件（含 `sdkconfig`、`.vscode`、原始压缩包）都保留原样。

---

## 目录结构

```text
资料/
├── README.md                      # 本文件：目录结构说明
├── 01-idf-learn-led/              # 自己封装的 BSP/LED 点灯工程
├── 02-zhengdian-self-led/         # 正点原子板：自己跟着写的点灯
├── 03-zhengdian-course-led/       # 正点原子官方课程源码 01_led
├── 04-official-blink/             # 乐鑫官方 blink + hello_world 示例
├── 05-beep-xl9555/                # XL9555 扩展 IO 驱动蜂鸣器
└── 06-ble-control-car/            # BLE 遥控小车（完整工程 + 原始资料）
```

---

## 1. `01-idf-learn-led/` —— 自己封装的 BSP/LED 点灯

**来源**：`y9000x:/home/rainboy/mycode/idf-learn/01_LED`
**目标芯片**：ESP32-S3 ｜ **点灯引脚**：`GPIO_NUM_1`

自己从零建的 ESP-IDF 工程，是这几个点灯工程里**结构最规整**的一个：LED 被抽成独立的 BSP 组件，`main.c` 只负责调用。

```text
01-idf-learn-led/
├── CMakeLists.txt                 # 顶层：set(EXTRA_COMPONENT_DIRS components/Middlewares)
├── components/
│   └── BSP/
│       ├── CMakeLists.txt
│       └── LED/
│           ├── led.c              # led_init() / led_task()
│           └── led.h              # #define LED_GPIO GPIO_NUM_1
├── main/
│   ├── CMakeLists.txt
│   ├── idf_component.yml
│   └── main.c                     # app_main: led_init() -> led_task()
├── partitions-16Mib.csv
├── sdkconfig / sdkconfig.old
└── .gitignore                     # build/
```

关键代码：

- `led.h`：`#define LED_GPIO GPIO_NUM_1`
- `led.c` 的 `led_init()` 用 `gpio_config_t` 配成 `GPIO_MODE_OUTPUT`，初始输出高电平（**负极驱动，高电平熄灭**）
- `led_task()` 是一个 FreeRTOS 任务，`vTaskDelay(pdMS_TO_TICKS(3500))` 翻转一次，`led_state ? 0 : 1` 低电平点亮

**注意点**：

- `main/idf_component.yml` 里残留了一行 `espressif/openai: ^1.1.0` 依赖（从别的模板带过来的），和点灯无关，可以删掉
- `compile_commands.json` 是指向 `build/compile_commands.json` 的软链接，因 `build/` 已被排除，克隆下来后是断链，`idf.py build` 一次即自动恢复

---

## 2. `02-zhengdian-self-led/` —— 正点原子板，自己跟写的点灯

**来源**：`y9000x:/home/rainboy/mycode/esp32/esp32s3-learn-zhengdian/01_LED`
**目标芯片**：ESP32-S3 ｜ **点灯引脚**：`GPIO_NUM_1`

与 `01-idf-learn-led` **文件内容逐字节相同**（已 `diff -r` 验证），是同一套代码在正点原子学习仓库里的副本。区别只在来源仓库的定位：`esp32s3-learn-zhengdian` 是整套 DNESP32S3 课程的学习仓库，这个是其中的第一个实验。

保留它的意义是对照：**同一份 BSP/LED 代码，分别放在"纯练习工程"和"整套课程仓库"里的组织方式**。

---

## 3. `03-zhengdian-course-led/` —— 正点原子官方课程源码 `01_led`

**来源**：`y9000x:/home/rainboy/mycode/esp32/esp32s3-learn-zhengdian/课程资料/1，课程源码/01_led`
**目标芯片**：ESP32-S3 ｜ **点灯引脚**：`GPIO_NUM_1`

官方原版，和上面两个的最大差别是**用宏封装了 GPIO 操作**，风格更接近传统单片机教程：

```c
// LED_TOGGLE()：读回当前电平再取反
#define LED_TOGGLE()  do { gpio_set_level(LED_GPIO_PIN, !gpio_get_level(LED_GPIO_PIN)); } while(0)

// LED(x)：x 为真则 PIN_SET，否则 PIN_RESET
#define LED(x)  do { x ? gpio_set_level(LED_GPIO_PIN, PIN_SET) : gpio_set_level(LED_GPIO_PIN, PIN_RESET); } while(0)
```

`main.c` 因此极短：

```c
void app_main(void)
{
    led_init();
    while (1) {
        LED_TOGGLE();
        vTaskDelay(500);     // 官方用 500ms，自己写的用 3500ms
    }
}
```

其他差异：

- `led_init()` 配的是 `GPIO_MODE_INPUT_OUTPUT`（**输入输出模式**，为了 `LED_TOGGLE()` 能读回电平），而 01/02 用的是 `GPIO_MODE_OUTPUT`；同时使能了内部上拉
- 带完整 `.vscode/`（`c_cpp_properties.json` / `launch.json`）和 `.devcontainer/`
- 分区表叫 `partitions-16MiB.csv`（01/02 是 `partitions-16Mib.csv`）
- 顶层 `project(00_basic)`，名字没改（课程源码的常见现象）

**这是学习宏封装 vs 任务封装两种风格的对照样本。**

---

## 4. `04-official-blink/` —— 乐鑫官方 blink + hello_world

**来源**：`y9000x:/home/rainboy/mycode/esp_test/offical-blink`
**来源仓库**：ESP-IDF 官方 `examples` 直接拷贝 ｜ **目标芯片**：ESP32-S3

这是**唯一一个"官方出品、跨芯片支持"的工程**，含两个子工程：

```text
04-official-blink/
├── README.md
├── .build-test-rules.yml
├── blink/
│   ├── CMakeLists.txt
│   ├── main/
│   │   ├── blink_example_main.c   # 核心：GPIO 闪烁 或 led_strip 驱动 WS2812
│   │   ├── Kconfig.projbuild      # BLINK_LED_TYPE / BLINK_GPIO 配置项
│   │   └── idf_component.yml      # espressif/led_strip: "^3.0.0"
│   ├── sdkconfig.defaults         # 本机配置：CONFIG_BLINK_LED_GPIO=y, CONFIG_BLINK_GPIO=8
│   ├── sdkconfig.defaults.esp32s3 # 板级默认：CONFIG_BLINK_LED_STRIP=y, GPIO38
│   ├── sdkconfig.defaults.esp32 / c3 / c5 / c6 / c61 / h2 / p4 / s2
│   ├── pytest_blink.py            # 官方 pytest 用例
│   └── README.md
└── hello_world/
    ├── main/hello_world_main.c
    ├── pytest_hello_world.py
    ├── sdkconfig.ci
    └── README.md
```

两个值得注意的点：

- **同一份代码双后端**：`blink_example_main.c` 里用 `#ifdef CONFIG_BLINK_LED_STRIP` 切换 `led_strip_new_rmt_device()`（WS2812 灯带）和纯 `gpio_set_level()` 两条路径，是学 "配置驱动编译" 的好例子
- **`sdkconfig.defaults.esp32s3` 里的版本差异说明**：
  ```
  # ESP32-S3-DevKitC v1.1 uses GPIO38 for the on-board LED
  # ESP32-S3-DevKitC v1.0 uses GPIO48 for the on-board LED
  ```
  本机 `sdkconfig.defaults` 被改成 `CONFIG_BLINK_LED_GPIO=y` + `GPIO=8`（外接 LED），和 `esp32s3` 板级默认（GPIO38 灯带）不一致，两者对照可以看清"默认值如何被覆盖"
- `blink/CMakeLists.txt` 里开了 `idf_build_set_property(MINIMAL_BUILD ON)`，只编 `main` 及其依赖，所以体积极小

---

## 5. `05-beep-xl9555/` —— XL9555 扩展 IO 驱动蜂鸣器

**来源**：`y9000x:/home/rainboy/mycode/esp32/esp32s3-learn-zhengdian/AI_code/beep_xl9555`
**目标芯片**：ESP32-S3 ｜ **板卡**：正点原子 DNESP32S3（带 XL9555 + ES8388）

这一份严格说不是"点灯"，但因为**和点灯工程能直接对照**所以收在这里：同样是"翻转一个 IO"，只是目标 IO 在 **I2C 扩展芯片 XL9555** 上，而不是 ESP32 自己的引脚。这也是点灯之后最自然的下一步。

```text
05-beep-xl9555/
├── CMakeLists.txt                 # project(beep_xl9555)
├── main/
│   ├── CMakeLists.txt
│   └── main.c                     # 全部逻辑都在这一个文件
├── partitions-16Mib.csv
├── sdkconfig
└── sdkconfig.defaults             # 16MB Flash + OCT PSRAM 80MHz
```

核心实现：

| 项 | 值 |
|---|---|
| I2C 端口 | `I2C_NUM_0` |
| SDA / SCL | `GPIO_NUM_41` / `GPIO_NUM_42` |
| I2C 频率 | 400 kHz |
| XL9555 地址 | `0x20` |
| 寄存器 | 输入 `0x00` / 输出 `0x02` / 配置 `0x06` |
| 蜂鸣器位 | `XL9555_BEEP_IO = 0x0008`（低电平响） |
| 功放使能位 | `XL9555_SPK_EN_IO = 0x0004` |

用新版 `driver/i2c_master.h`（`i2c_new_master_bus` / `i2c_master_bus_add_device`），读改写：

```c
xl9555_pin_write(XL9555_BEEP_IO, 0);   // 响 120ms
vTaskDelay(pdMS_TO_TICKS(BEEP_ON_TIME_MS));
xl9555_pin_write(XL9555_BEEP_IO, 1);   // 停 1200ms
vTaskDelay(pdMS_TO_TICKS(BEEP_OFF_TIME_MS));
```

**和点灯工程的关键区别**：`gpio_set_level()` 变成"先 `i2c_master_transmit_receive()` 读回 16 位输出状态 → 改位 → 再写回"，讲"为什么扩展 IO 比直连 GPIO 麻烦"时可以直接拿这个对比。

同目录下还有 `AI_code/` 里的 `speaker_tone_es8388`、`audio_loopback_es8388`、`xiaozhi_audio_teach_es8388`（ES8388 音频实验），本次未收录。

---

## 6. `06-ble-control-car/` —— BLE 遥控小车

**来源**：`y9000x:/home/rainboy/nas-backup/data/数据/Level_1/电子创客/救援小车`
**目标芯片**：源码内 `sdkconfig` 为 **ESP32-C3**（代码同时支持 C3 / S3）

唯一一个**功能完整**的工程：BLE 遥控 + 4 路电机 + 状态灯 + WiFi 配网 + 路径记录。原始资料和源码都在，压缩包按原样保留。

```text
06-ble-control-car/
├── 说明文档.txt                   # 硬件接线说明（引脚对照）
├── APP：ble-control-car.zip        # 配套手机 APP
├── ble-control-car.zip            # 工程打包
├── ESP32：ble-control-car.zip      # 工程打包
└── ESP32：ble-control-car/         # 源码工程（目录名为原始名，含全角冒号）
    ├── CMakeLists.txt             # project(ble-control-car)
    ├── main/
    │   ├── main.c                 # 827 行：车体状态机 + 导航列表 + BLE 回调
    │   ├── gatt_svr.c/.h          # NimBLE GATT 服务（接收遥控指令）
    │   ├── led.h                  # 状态灯（header-only 实现）
    │   ├── Kconfig.projbuild      # 电机引脚 / LED 类型 / 板型选择
    │   └── idf_component.yml      # led_strip ^2.0.0, idf >= 5.1.0
    ├── sdkconfig                  # 当前配置为 esp32c3
    ├── sdkconfig.old
    ├── .devcontainer/ .vscode/ .clangd
    └── esp32救援源代码.zip
```

### 硬件配置（见 `说明文档.txt` 与 `main/Kconfig.projbuild`）

| 项 | ESP32-S3 | ESP32-C3 |
|---|---|---|
| 电机 1/2/3/4 | GPIO 7 / 6 / 5 / 4 | GPIO 5 / 4 / 8 / 9 |
| LED | GPIO 48（WS2812 灯带） | GPIO 12 / 13（两个单色灯） |
| 配网按键 | GPIO 12（`KEY_SETUP`，长按 3s 进配网） | 同 |

对应 Kconfig 项：`MOTOR_A_IN1/IN2`、`MOTOR_B_IN3/IN4`、`BOARD_TYPE_S3` / `BOARD_TYPE_C3`、`LED_PIN` / `LED_PIN1` / `LED_PIN2`、`MAX_LEDS`、`LONG_PRESS_TIME`。**换板子只改 menuconfig，不用改代码** —— 这是它比前面几个点灯工程"工程化"的地方。

### 代码结构（`main/main.c`）

- **状态机**：`car_state_t` = `STOP / RUN_UP / RUN_DOWN / RUN_LEFT / RUN_RIGHT`
- **模式**：`car_mode_t` = `REMOTE_CONTROL / STUDY / NAV / AUTO / BACK_SAME_WAY`（STUDY 模式录制路径，BACK_SAME_WAY 原路返回）
- **路径记录**：手写动态数组 `CarNavList`（create / append / get / insert / remove / clear / destroy），元素是 `{car_state_t state, double time}`
- **BLE**：NimBLE 协议栈，`gatt_svr.c` 提供 GATT 服务，`bleprph` 风格
- **LED**：`led.h` 是 header-only，用 `#if CONFIG_BOARD_TYPE_C3` / `S3` 分支 —— C3 走 `gpio_set_level()`，S3 走 `led_strip_new_rmt_device()` + `set_led_red/blue/green/cyan()` 颜色函数（绿=MQTT 连上，蓝=AP 配网中）
- 有 `CONFIG_PM_ENABLE` 下的 `esp_pm` 电源管理分支

**注意点**：`main.c` 里 `void car_up(void);` 这类前置声明和定义同文件；`CAR_TURN_AROUND 400` 是"按时间控制掉头"的实验参数，需要按电池电压和电机实际转速调。

---

## 横向对照：5 个点灯工程有什么不一样

| | 01 idf-learn | 02 zhengdian-self | 03 课程源码 | 04 官方 blink | 05 beep_xl9555 |
|---|---|---|---|---|---|
| 来源 | 自己建 | 自己建（副本） | 正点原子官方 | 乐鑫官方 | 自己写 |
| IO 位置 | ESP32 GPIO1 | ESP32 GPIO1 | ESP32 GPIO1 | GPIO8 / GPIO38 | XL9555 P0.3（I2C） |
| 驱动方式 | `gpio_config` 输出 | 同 01 | `GPIO_MODE_INPUT_OUTPUT` | GPIO 或 led_strip（配置切换） | I2C 读改写 |
| 翻转周期 | 3500 ms | 3500 ms | 500 ms | 1000 ms（官方 Kconfig） | 120 ms 响 / 1200 ms 停 |
| 抽象方式 | BSP 组件 + FreeRTOS 任务 | 同 01 | `LED()` / `LED_TOGGLE()` 宏 | `#ifdef CONFIG_BLINK_*` 编译期分支 | 单文件函数 |
| 跨芯片 | 否（S3） | 否（S3） | 否（S3） | **是**（10+ 目标） | 否（S3） |
| 状态灯 | 无 | 无 | 无 | 无 | 蜂鸣器（非灯） |

三条演进线索：

1. **抽象层次**：宏封装（03）→ 组件 + 任务（01/02）→ 编译期配置分支（04）
2. **IO 距离**：ESP32 本地 GPIO（01~04）→ I2C 远程扩展 IO（05）
3. **代码规模**：单函数（03/05）→ 组件拆分（01/02）→ 完整应用（06 小车 827 行 main.c）

---

## 复现环境

- **ESP-IDF**：`v5.5.4`（`y9000x:/home/rainboy/.espressif/v5.5.4`）
  激活：`source ~/.espressif/tools/activate_idf_v5.5.4.sh`
- **芯片**：绝大多数为 **ESP32-S3**；`06` 的 `sdkconfig` 为 **ESP32-C3**（代码两者都支持）
- **注意**：`01`/`02` 的 `idf_component.yml` 里残留 `espressif/openai` 依赖；`03` 的顶层 `project()` 名仍为 `00_basic`

常规构建（在任一工程目录下）：

```bash
source ~/.espressif/tools/activate_idf_v5.5.4.sh
idf.py set-target esp32s3      # 06 小车按现有配置用 esp32c3
idf.py menuconfig              # 需要时改引脚
idf.py build flash monitor
```

---

## 未收录的相关工程（在 `y9000x` 上）

以下几处与本目录相关但体积大 / 不属于"点灯或小车"，留在原机：

| 位置 | 内容 |
|---|---|
| `~/mycode/esp32/esp32-audio-learn/` | ESP-IDF 音频学习 9 课（PCM5102 / INMP441 / VAD / Opus） |
| `~/mycode/esp32/esp32s3-learn-zhengdian/课程资料/1，课程源码/` | 正点原子官方 18 课其余 17 课（key / uart / tim / iic / spi lcd / adc / WiFi / BLE） |
| `~/mycode/esp32/esp32s3-learn-zhengdian/AI_code/` | ES8388 音频实验 4 个工程 |
| `~/__git__/xiaozhi-esp32/` | 小智 AI 项目 |
| `~/mycode/MyChip/` | ESP8266 实用项目（**Arduino 框架**，非 IDF） |

---

## 溯源

| 本目录 | `y9000x` 原始路径 |
|---|---|
| `01-idf-learn-led/` | `/home/rainboy/mycode/idf-learn/01_LED/` |
| `02-zhengdian-self-led/` | `/home/rainboy/mycode/esp32/esp32s3-learn-zhengdian/01_LED/` |
| `03-zhengdian-course-led/` | `/home/rainboy/mycode/esp32/esp32s3-learn-zhengdian/课程资料/1，课程源码/01_led/` |
| `04-official-blink/` | `/home/rainboy/mycode/esp_test/offical-blink/` |
| `05-beep-xl9555/` | `/home/rainboy/mycode/esp32/esp32s3-learn-zhengdian/AI_code/beep_xl9555/` |
| `06-ble-control-car/` | `/home/rainboy/nas-backup/data/数据/Level_1/电子创客/救援小车/` |

同步方式（已排除 `build/`、`managed_components/`、`.git/`）：

```bash
rsync -a --exclude=build/ --exclude=managed_components/ --exclude=.git/ \
  'y9000x:<原始路径>/' <本目录>/<目标目录>/
```
