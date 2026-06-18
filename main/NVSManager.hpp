//
// Created by GZX on 2026/6/18.
//
#pragma once
#include <string>

constexpr const char *AI_DEFAULT_URL = "https://api.siliconflow.cn/v1/chat/completions";

class NVSManager {
public:
    bool init();

    // WiFi
    bool getWiFiCred(std::string &ssid, std::string &pwd);
    void setWiFiCred(const std::string &ssid, const std::string &pwd);
    void clearWiFiCred();

    // AI
    bool getAIConfig(std::string &url, std::string &apiKey, std::string &model);
    void setAIConfig(const std::string &url, const std::string &apiKey, const std::string &model);
    bool getAIStreamEnabled();
    void setAIStreamEnabled(bool enabled);
    void clearAIConfig();
};