# Esp32-AiControlHardware 🤖

> ESP32 硬件控制项目，集成 AI 语音交互，通过智能指令控制各类外设

---

## 📖 项目简介

**Esp32-AiControlHardware** 是一个基于 ESP32 的智能硬件控制平台，通过集成大语言模型 API，实现自然语言驱动的设备控制。用户可以通过语音或文本输入指令，系统自动解析并执行对应的硬件操作，让智能家居控制更加自然、便捷。

---

## ✨ 功能特性

- 🎤 **AI 语音交互**：集成 SiliconFlow AI API，支持自然语言指令解析
- 🌐 **Web 控制面板**：内置 HTTP 服务器，提供可视化交互界面
- 📡 **Wi-Fi 联网**：支持无线网络连接，实现远程控制
- 🔌 **GPIO 硬件控制**：支持 D2、Trigger 等引脚控制，可扩展各类外设
- 💾 **NVS 存储管理**：参数持久化存储，断电不丢失
- 🚀 **实时流式响应**：支持 AI 回复流式输出，交互体验更流畅

---

## 🧰 硬件要求

| 组件 | 说明 |
|------|------|
| **主控** | ESP32 系列开发板 |
| **引脚** | D2（GPIO 2）、Trigger（GPIO 13） |
| **外设** | 可根据需求扩展继电器、传感器、LED 等 |

---

## 📦 软件依赖

- **ESP-IDF** v5.0 或更高版本
- **C++17** 编译器
- **CMake** 构建系统
- **SiliconFlow API Key**（用于 AI 功能）

---

## 🔧 安装与构建

### 1. 克隆仓库

```bash
git clone https://github.com/GZXSSS/Esp32-AiControlHardware.git
cd Esp32-AiControlHardware
2. 配置 ESP-IDF 环境
请参考 ESP-IDF 官方文档 完成环境配置。

3. 配置项目
bash
idf.py set-target esp32
idf.py menuconfig
在 menuconfig 中配置 Wi-Fi SSID/密码以及 AI API Key。

4. 编译与烧录
bash
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
🚀 使用说明
本地控制
烧录完成后，ESP32 将自动连接 Wi-Fi 并启动 Web 服务器。在浏览器中访问 ESP32 的 IP 地址，即可打开控制面板。

AI 语音控制
通过 Web 界面或串口发送自然语言指令，例如：

“打开灯光”

“关闭风扇”

“启动流水灯”

系统将自动解析指令并执行对应的 GPIO 操作。

核心 API
模块	功能
AIController	AI 对话管理，支持同步/流式调用
GPIOManager	GPIO 初始化与引脚控制
WebServerManager	HTTP 服务器与路由管理
NVSManager	非易失性存储管理
📁 项目结构
text
Esp32-AiControlHardware/
├── main/
│   ├── main/                    # 核心源码
│   │   ├── AIController.cpp/hpp # AI 控制器
│   │   ├── GPIOManager.cpp/hpp  # GPIO 管理
│   │   ├── WebServerManager.cpp/hpp # Web 服务器
│   │   ├── NVSManager.cpp/hpp   # 存储管理
│   │   ├── WiFiManager.cpp/hpp  # Wi-Fi 管理
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
🤝 贡献指南
欢迎提交 Issue 和 Pull Request！

Fork 本仓库

创建您的特性分支 (git checkout -b feature/AmazingFeature)

提交您的更改 (git commit -m 'Add some AmazingFeature')

推送到分支 (git push origin feature/AmazingFeature)

打开一个 Pull Request

📄 许可证
本项目采用 MIT License 开源协议，详情请参阅 LICENSE 文件。

🙏 致谢
ESP-IDF — 乐鑫官方物联网开发框架

SiliconFlow — AI API 服务支持

<div align="center">
⭐ 如果这个项目对你有帮助，请给一个 Star！

</div> ```
