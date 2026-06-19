# Esp32-AiControlHardware 🤖

> ESP32 hardware control project, integrating AI voice interaction to control various peripherals through intelligent commands

(English | [中文](README.md))

---

## 📖 Project Introduction

**Esp32-AiControlHardware** is an intelligent hardware control platform based on ESP32. By integrating a large language model API (OpenAI-like), it enables natural language-driven device control. Users can operate hardware via voice, text, or serial commands; the system automatically parses the instructions and executes the corresponding GPIO operations, making smart home control more natural and convenient.

---

## ✨ Features

- 🎤 **AI Language Interaction**: Integrated SiliconFlow AI API, supporting natural language command parsing
- 🌐 **Web Control Panel**: Built‑in HTTP server providing a visual interactive interface
- 📡 **Wi‑Fi Connectivity**: Supports wireless network connection for remote control
- 🔌 **GPIO Hardware Control**: Supports D2, Trigger and other pin controls, expandable to various peripherals
- 💾 **NVS Storage Management**: Persistent parameter storage, no data loss after power‑off
- 🚀 **Real‑time Streaming Response**: Supports streaming AI replies for a smoother interactive experience
- 🔄 **One‑click Reset to Provisioning**: Short D13 (GPIO 13) to GND to clear NVS storage and automatically switch to Wi‑Fi AP provisioning mode
- 📟 **Serial Command Control**: Supports sending ASCII commands via UART to directly control GPIO states without network connection

---

## 🧰 Hardware Requirements

| Component | Description |
|-----------|-------------|
| **Main Controller** | ESP32 series development board |
| **Core Pins** | D2 (GPIO 2): main output control pin / D13 (GPIO 13): provisioning reset trigger pin |
| **Reset Method** | Short D13 to GND for about 2~3 seconds to trigger NVS clearing and enter AP mode |
| **Peripherals** | Can be expanded with relays, sensors, LEDs, etc. as needed |

---

## 📦 Software Dependencies

- **ESP‑IDF** v5.0 or higher
- **C++17** compiler
- **CMake** build system
- **SiliconFlow API Key** (for AI features, optional)

---

## 🔧 Installation and Build

### 1. Clone the Repository

```bash
git clone https://github.com/GZXSSS/Esp32-AiControlHardware.git
cd Esp32-AiControlHardware
```

### 2. Configure the ESP‑IDF Environment

Please refer to the [ESP‑IDF official documentation](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/get-started/) to set up the environment.

### 3. Configure the Project

```bash
idf.py set-target esp32
idf.py menuconfig
```

### 4. Build and Flash

```bash
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

---

## 🚀 Usage Instructions

### Provisioning Mode (AP Mode)

When you need to change Wi‑Fi or forget the password, there is no need to re‑flash the firmware:

1. Short the **D13 (GPIO 13)** pin of the ESP32 to **GND** (using a Dupont wire or a button).
2. Keep the short for about **2~3 seconds** (or the device will automatically trigger after detecting the low level).
3. The system will automatically clear the Wi‑Fi configuration stored in NVS and restart.
4. After restarting, it enters AP mode with the hotspot name `ESP32_SetUp`.
5. Connect your phone/computer to this hotspot, visit `192.168.4.1` in the browser to enter the provisioning page, and enter the new Wi‑Fi credentials.

### Local Web Control

After normal flashing and networking, the ESP32 will automatically connect to the configured Wi‑Fi. Open the IP address printed over serial in your browser to access the interactive control panel.

### UART Serial Command Control

Control hardware directly via serial (USB connection) without relying on Wi‑Fi or the web interface:

- **Baud rate**: `115200` (default)
- **Data bits**: 8
- **Stop bits**: 1
- **Line ending**: `\r\n` or `\n`

**Basic command examples** (the exact command set is based on the `uart_handler` implementation in the code):
- Send `on` or `1` → Set D2 pin high (e.g., turn on an LED)
- Send `off` or `0` → Set D2 pin low (e.g., turn off an LED)
- Send `liu` → Toggle the current state of D2 repeatedly (chaser/flashing effect)

### AI Language Control (Advanced)

Send natural language via the web interface or serial (requires API key configuration), for example:

- *"Turn on the light"*
- *"Turn off the light"*
- *"Start the chaser light"*

The system will automatically parse the intent and execute the corresponding hardware operation.

### Core API

| Module | Function |
|--------|----------|
| `AIController` | AI conversation management, supporting synchronous/streaming calls |
| `GPIOManager` | GPIO initialisation and pin control (including D13 reset detection) |
| `WebServerManager` | HTTP server and route management (including provisioning page) |
| `NVSManager` | Non‑volatile storage management (Wi‑Fi info, parameter read/write and clearing) |

---

## 📁 Project Structure

```
Esp32-AiControlHardware/
├── main/
│   ├── main/                    # Core source code
│   │   ├── AIController.cpp/hpp # AI controller
│   │   ├── GPIOManager.cpp/hpp  # GPIO management (including D13 reset logic)
│   │   ├── WebServerManager.cpp/hpp # Web server (including AP provisioning mode)
│   │   ├── NVSManager.cpp/hpp   # Storage management (including clearing interface)
│   │   ├── WiFiManager.cpp/hpp  # Wi‑Fi management (including AP/STA switching)
│   │   ├── UartHandler.cpp/hpp  # Serial command parsing and control
│   │   ├── App.cpp/hpp          # Main application logic
│   │   ├── app_main.cpp         # Entry function
│   │   ├── CMakeLists.txt       # Build configuration
│   │   └── idf_component.yml    # Component configuration
│   ├── html/                    # Web interface
│   │   ├── chat.html            # AI chat interface
│   │   └── root.html            # Control panel homepage
│   ├── CMakeLists.txt
│   └── README.md
└── LICENSE
```

---

## 🤝 Contribution Guidelines

Issues and Pull Requests are welcome!

1. Fork this repository
2. Create your feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

---

## 📄 License

This project is open‑sourced under the **MIT License** – see the [LICENSE](LICENSE) file for details.

---

## 🙏 Acknowledgements

- [ESP‑IDF](https://github.com/espressif/esp-idf) — Espressif's official IoT development framework
- [SiliconFlow](https://siliconflow.cn) — AI API service support

---

<div align="center">

**⭐ If this project helps you, please give it a Star!**

</div>
