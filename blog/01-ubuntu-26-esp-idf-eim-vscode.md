---
title: "在 Ubuntu 26.04 安装 ESP-IDF、EIM 和 VS Code"
date: 2026-09-23
draft: true
toc: true
tags: ["ESP32", "ESP-IDF", "Ubuntu", "VS Code"]
---

这一篇先把 ESP32 开发环境装好：用 **EIM（ESP-IDF Installation Manager）** 安装 ESP-IDF 和工具链，再让 VS Code 的 ESP-IDF 插件识别它，最后创建一个项目并编译验证。这里的 Ubuntu 26 指 **Ubuntu 26.04 桌面版**；命令适用于常见的 `amd64` 和 `arm64` 架构。实际刷机还需要一块 ESP32 开发板和数据线。

> [!INFO] 版本说明
> 下文按 2026-09-23 查阅的乐鑫和 VS Code 官方文档编写。EIM 的界面和最新稳定版号会变化，所以安装时选择界面显示的稳定版，不把某个版本号写死。文章中的 Ubuntu 命令尚未在 Ubuntu 26.04 实机上逐条执行；每一步都给出可以在目标机器上核对的结果。

## 先弄清三个东西

| 名称 | 作用 |
| --- | --- |
| ESP-IDF | 乐鑫的开发框架，包含项目模板、组件、`idf.py` 等 |
| EIM | 安装和管理 ESP-IDF、Python 环境及交叉编译工具链 |
| VS Code + ESP-IDF 插件 | 编辑项目、选择芯片与串口、编译、烧录和查看日志 |

ESP-IDF 6.0 起，乐鑫的 Linux 入门文档把 EIM 列为默认安装方式。下面先用 EIM 安装一次，VS Code 只连接这套现有环境，避免重复下载工具链。参见[乐鑫 Linux 安装说明](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/linux-setup.html)。

## 1. 确认 Ubuntu 和架构

打开终端：

```bash
cat /etc/os-release
dpkg --print-architecture
python3 --version
```

检查 `VERSION_ID` 是否为 `26.04`，架构是否为 `amd64` 或 `arm64`。如果机器还没有装 Git，可先运行 `sudo apt update && sudo apt install git`。使用下面的 EIM APT 包时，乐鑫文档说明可跳过手动准备 EIM 前置依赖；若安装器报告缺少组件，再按报错补装，不必先把整套 ESP-IDF 依赖装两遍。

## 2. 安装 EIM

乐鑫提供 Debian/Ubuntu 的 APT 仓库。下面安装带图形界面的 `eim`，它也包含 CLI：

```bash
echo "deb [trusted=yes] https://dl.espressif.com/dl/eim/apt/ stable main" | sudo tee /etc/apt/sources.list.d/espressif.list
sudo apt update
sudo apt install eim
eim --help
```

`eim --help` 能输出命令帮助，就说明可执行文件已经进入 `PATH`。只在纯命令行服务器上使用时，可以把 `sudo apt install eim` 换成 `sudo apt install eim-cli`。

> [!WARNING] APT 仓库信任方式
> 上面的仓库配置照录自[乐鑫官方安装文档](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/linux-setup.html)。其中 `trusted=yes` 会让 APT 对**这个仓库**跳过通常的包签名校验；先确认域名确实是 `dl.espressif.com`，并只在接受此信任方式时使用。若不愿添加此仓库，可从[乐鑫 EIM 下载页](https://dl.espressif.com/dl/eim/)获取适合自己架构的安装包，按下载页说明安装。

## 3. 用 EIM 安装 ESP-IDF

### 图形界面

在终端运行 `eim`，或从应用菜单打开 ESP-IDF Installation Manager。依次选择 **New Installation → Start Installation → Easy Installation → Start Easy Installation**，使用当前稳定版和默认目录。通过前置检查后点击 **Start Installation**。等到 **Installation Complete**，再到安装管理页面确认版本已出现。若下载或安装失败，先打开界面底部的 **Logs** 看具体错误。

### 命令行

在有网络的终端运行：

```bash
eim install
```

这会按默认设置安装当前稳定版。希望交互选择版本、镜像、特性或安装位置时，改用：

```bash
eim wizard
```

例如项目确实需要某个特定版本，可先用 `eim --help` 核对本机参数，再运行 `eim install -i v5.4.2`。不要同时启动 GUI 和 CLI 对同一个安装目录进行安装。EIM 成功时会报告 `Successfully installed IDF`；还应检查生成的安装记录：

```bash
test -f "$HOME/.espressif/tools/eim_idf.json" && echo 'EIM 安装记录存在'
find "$HOME/.espressif/tools" -maxdepth 1 -name 'activate_idf_*.sh' -print
```

EIM 默认在 `~/.espressif/tools/eim_idf.json` 写入供 VS Code 识别的配置；使用自定义目录时，应以安装器显示的实际路径为准。上面的激活脚本按安装版本命名。它必须通过 `source` 加载到**当前**终端：

```bash
source "$HOME/.espressif/tools/activate_idf_版本号.sh"
idf.py --version
```

把 `版本号` 替换为刚才 `find` 输出的真实文件名，例如 `activate_idf_v6.0.sh`；不要直接执行脚本，也不要随意把某个版本的激活命令永久写进 `~/.bashrc`。每次新开终端需要使用 `idf.py` 时，重新 `source` 对应脚本。EIM 的[激活脚本说明](https://docs.espressif.com/projects/idf-im-ui/en/latest/after_installing.html)也说明它同时支持 Bash 和 Zsh。

## 4. 安装 VS Code

这里使用微软官方 `.deb` 包，避免 Snap 沙箱给 EIM 图形界面和串口访问带来额外问题。打开 [VS Code 下载页](https://code.visualstudio.com/Download)，选择与你的 `dpkg --print-architecture` 对应的 **Debian/Ubuntu `.deb`** 包，下载后在包所在目录执行：

```bash
sudo apt install ./code_*.deb
code --version
```

若当前目录中有多个 `code_*.deb`，请把通配符改成要安装的准确文件名。官方说明 `.deb` 安装过程会询问是否添加 VS Code 的 APT 软件源，选择添加后，以后可以通过系统更新获取新版。具体步骤见[VS Code Linux 安装文档](https://code.visualstudio.com/docs/setup/linux)。

## 5. 安装并配置 ESP-IDF 插件

打开 VS Code，按 `Ctrl+Shift+X` 进入扩展市场，搜索 **ESP-IDF**，确认发布者是 **Espressif Systems** 后安装。也可以在终端执行：

```bash
code --install-extension espressif.esp-idf-extension
```

安装完成后，按 `F1` 打开命令面板，运行 **ESP-IDF: Select Current ESP-IDF Version**，从列表选择刚才 EIM 安装的版本。再运行 **ESP-IDF: Doctor Command**，检查插件、Python 环境和工具链路径。插件会从 EIM 的 `eim_idf.json` 自动发现安装记录；如果列表为空，先确认文件存在，再在设置里指定 `idf.eimIdfJsonPath` 为真实的 `eim_idf.json` 绝对路径。参见[插件安装与配置文档](https://docs.espressif.com/projects/vscode-esp-idf-extension/en/latest/installation.html)。

> [!TIP] 不要重复安装
> 如果已经用 EIM 安装 ESP-IDF，插件提示打开 Installation Manager 时，无需再安装一遍。先执行 **Select Current ESP-IDF Version** 选中现有环境。VS Code 集成终端若要手动运行 `idf.py`，同样需要在该终端 `source` 对应的激活脚本。

## 6. 建立项目并验证

按 `F1` 运行 **ESP-IDF: New Project**，选择 EIM 安装的版本，再选择 `get-started/hello_world` 示例。填写项目名和保存位置，**Target** 要与开发板的实际芯片型号一致，例如经典 ESP32 选 `esp32`，ESP32-C3 选 `esp32c3`；不确定时看开发板或芯片丝印。点击 **Create Project**，随后打开新项目。

运行 **ESP-IDF: Build your Project**。编译完成并显示 `Building done`，才算软件环境的第一轮验证通过；安装器完成或 Doctor 无报错都不能代替一次真实编译。[新建项目](https://docs.espressif.com/projects/vscode-esp-idf-extension/en/latest/startproject.html)和[编译](https://docs.espressif.com/projects/vscode-esp-idf-extension/en/latest/buildproject.html)的界面步骤可对照官方文档。

有开发板时，用**支持数据传输**的 USB 线连接，运行 **ESP-IDF: Select Port to Use**，选择对应的 `/dev/ttyUSB*` 或 `/dev/ttyACM*`，再运行 **ESP-IDF: Flash your Project**，一般选 `UART`。烧录成功后用插件的 Monitor 查看启动输出。也可在已经激活 ESP-IDF 的终端、项目根目录执行：

```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

`/dev/ttyUSB0` 仅为例子，应换成设备实际端口。`flash` 会自动编译；看到 `Hello world!` 之类的示例日志才说明编译、串口、烧录和程序运行都走通。相关命令见[乐鑫的第一个项目指南](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/start-project.html)。

## 常见问题

### VS Code 找不到已安装的 ESP-IDF

先检查 `~/.espressif/tools/eim_idf.json` 是否存在，再用 **ESP-IDF: Select Current ESP-IDF Version** 选择。自定义目录安装时，把 `idf.eimIdfJsonPath` 指向实际 JSON 文件。最后运行 **ESP-IDF: Doctor Command** 查看插件报告，不要凭猜测手写 `IDF_PATH`。

### `idf.py: command not found`

新终端尚未激活环境。用 `find "$HOME/.espressif/tools" -maxdepth 1 -name 'activate_idf_*.sh' -print` 找到脚本，`source` 正确版本后再试 `idf.py --version`。

### 串口 `Permission denied`

先用 `ls -l /dev/ttyUSB0`（或实际端口）查看设备所属组。Ubuntu 上常见为 `dialout`，若确实如此，运行：

```bash
sudo usermod -aG dialout "$USER"
```

随后**退出登录并重新登录**，用 `groups` 确认当前会话已包含 `dialout`，再重新插拔开发板并烧录。不要用 `sudo idf.py` 掩盖权限问题。如果设备组不是 `dialout`，按实际组处理。参见[乐鑫串口权限说明](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/start-project.html)。

### EIM 窗口在 VS Code 中打不开

先在系统终端直接运行 `eim`；如果通过 Snap 安装了 VS Code，留意沙箱限制。乐鑫的[插件排错文档](https://docs.espressif.com/projects/vscode-esp-idf-extension/en/latest/troubleshooting.html)建议改用官方 `.deb` 版本，并可通过 **ESP-IDF: Doctor Command** 定位 EIM 路径问题。

## 参考资料

- [乐鑫：Linux 上安装 ESP-IDF 与 EIM](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/linux-setup.html)
- [乐鑫：EIM 安装后的激活脚本](https://docs.espressif.com/projects/idf-im-ui/en/latest/after_installing.html)
- [乐鑫：VS Code ESP-IDF 插件安装](https://docs.espressif.com/projects/vscode-esp-idf-extension/en/latest/installation.html)
- [微软：在 Linux 安装 VS Code](https://code.visualstudio.com/docs/setup/linux)
