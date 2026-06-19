# Esp32-AiControlHardware 🤖

> ESP32 硬件控制项目，集成 AI 语言交互，通过智能指令控制各类外设

(中文 | [English](README_EN.md))
---

## 📖 项目简介

**Esp32-AiControlHardware** 是一个基于 ESP32 的智能硬件控制平台，通过集成大语言模型 API (类OPENAI) ，实现自然语言驱动的设备控制。用户可以通过语音、文本或串口指令操作硬件，系统自动解析并执行对应的 GPIO 操作，让智能家居控制更加自然、便捷。

---

## ✨ 功能特性

- 🎤 **AI 语言交互**：集成 SiliconFlow AI API，支持自然语言指令解析
- 🌐 **Web 控制面板**：内置 HTTP 服务器，提供可视化交互界面
- 📡 **Wi-Fi 联网**：支持无线网络连接，实现远程控制
- 🔌 **GPIO 硬件控制**：支持 D2、Trigger 等引脚控制，可扩展各类外设
- 💾 **NVS 存储管理**：参数持久化存储，断电不丢失
- 🚀 **实时流式响应**：支持 AI 回复流式输出，交互体验更流畅
- 🔄 **一键重置配网**：短接 D13 (GPIO 13) 与 GND 即可清空 NVS 存储并自动切换至 Wi-Fi AP 配网模式
- 📟 **串口指令控制**：支持通过 UART 发送 ASCII 指令，无需联网即可直接控制 GPIO 状态

---

## 🧰 硬件要求

| 组件 | 说明 |
|------|------|
| **主控** | ESP32 系列开发板 |
| **核心引脚** | D2（GPIO 2）：主控输出引脚 / D13（GPIO 13）：配网重置触发引脚 |
| **重置方式** | 短接 D13 与 GND 约 2~3 秒，触发 NVS 清除并进入 AP 模式 |
| **外设** | 可根据需求扩展继电器、传感器、LED 等 |

---

## 📦 软件依赖

- **ESP-IDF** v5.0 或更高版本
- **C++17** 编译器
- **CMake** 构建系统
- **SiliconFlow API Key**（用于 AI 功能，可选）

---

## 🔧 安装与构建

### 1. 克隆仓库

```bash
git clone https://github.com/GZXSSS/Esp32-AiControlHardware.git
cd Esp32-AiControlHardware
```

### 2. 配置 ESP-IDF 环境

请参考 [ESP-IDF 官方文档](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/get-started/) 完成环境配置。

### 3. 配置项目

```bash
idf.py set-target esp32
idf.py menuconfig
```

### 4. 编译与烧录

```bash
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

---

## 🚀 使用说明

### 配网模式（AP 模式）

当需要更换 Wi-Fi 或忘记密码时，无需重新烧录固件：

1. 将 ESP32 的 **D13 (GPIO 13)** 引脚与 **GND** 短接（可用杜邦线或按键）。
2. 保持短接状态约 **2~3 秒**（或设备检测到低电平后自动触发）。
3. 系统将自动清空 NVS 中存储的 Wi-Fi 配置并重启。
4. 重启后进入 AP 模式，热点名称为 `ESP32_SetUp`。
5. 使用手机/电脑连接该热点，在浏览器访问 `192.168.4.1` 进入配网页面，输入新的 Wi-Fi 信息即可。

### 本地 Web 控制

正常烧录并联网后，ESP32 将自动连接已配置的 Wi-Fi。在浏览器中访问串口打印的 IP 地址，即可打开交互式控制面板。

### UART 串口指令控制

无需依赖 Wi-Fi 或 Web 界面，直接通过串口（USB 连接）发送指令即可控制硬件：

- **波特率**：`115200` （默认）
- **数据位**：8
- **停止位**：1
- **换行符**：`\r\n` 或 `\n`

**基础指令示例**（具体指令集以代码中的 `uart_handler` 实现为准）：
- 发送 `on` 或 `1` → 将 D2 引脚拉高（例如打开 LED）
- 发送 `off` 或 `0` → 将 D2 引脚拉低（例如关闭 LED）
- 发送 `liu` → 循环翻转 D2 引脚当前状态(流水灯)

### AI 语言控制（进阶）

通过 Web 界面或串口发送自然语言（需配置 API Key），例如：

- *“打开灯光”*
- *“关闭灯光”*
- *“启动流水灯”*

系统将自动解析意图并执行对应的硬件操作。

### 核心 API

| 模块 | 功能 |
|------|------|
| `AIController` | AI 对话管理，支持同步/流式调用 |
| `GPIOManager` | GPIO 初始化与引脚控制（含 D13 重置检测） |
| `WebServerManager` | HTTP 服务器与路由管理（含配网页面） |
| `NVSManager` | 非易失性存储管理（Wi-Fi 信息、参数读写与清除） |

---

## 📁 项目结构

```
Esp32-AiControlHardware/
├── main/
│   ├── main/                    # 核心源码
│   │   ├── AIController.cpp/hpp # AI 控制器
│   │   ├── GPIOManager.cpp/hpp  # GPIO 管理（含 D13 重置逻辑）
│   │   ├── WebServerManager.cpp/hpp # Web 服务器（含配网 AP 模式）
│   │   ├── NVSManager.cpp/hpp   # 存储管理（含清空接口）
│   │   ├── WiFiManager.cpp/hpp  # Wi-Fi 管理（含 AP/STA 切换）
│   │   ├── UartHandler.cpp/hpp  # 串口指令解析与控制
│   │   ├── App.cpp/hpp          # 应用主逻辑
│   │   ├── app_main.cpp         # 入口函数
│   │   ├── CMakeLists.txt       # 构建配置
│   │   └── idf_component.yml    # 组件配置
│   ├── html/                    # Web 界面
│   │   ├── chat.html            # AI 对话界面
│   │   └── root.html            # 控制面板首页
│   ├── CMakeLists.txt
│   └── README.md
└── LICENSE
```

---

## 🤝 贡献指南

欢迎提交 Issue 和 Pull Request！

1. Fork 本仓库
2. 创建您的特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交您的更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 打开一个 Pull Request

---

## 📄 许可证

本项目采用 **MIT License** 开源协议，详情请参阅 [LICENSE](LICENSE) 文件。

---

## 🙏 致谢

- [ESP-IDF](https://github.com/espressif/esp-idf) — 乐鑫官方物联网开发框架
- [SiliconFlow](https://siliconflow.cn) — AI API 服务支持

---

<div align="center">

**⭐ 如果这个项目对你有帮助，请给一个 Star！**

</div>
