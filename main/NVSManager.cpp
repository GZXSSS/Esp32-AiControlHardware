//
// Created by GZX on 2026/6/18.
//
#include "NVSManager.hpp"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <cstring>

static const char *TAG = "NVS";
static const char *NVS_NAMESPACE = "config";
static const char *NVS_KEY_AI_URL = "url";
static const char *NVS_KEY_AI_API_KEY = "api_key";
static const char *NVS_KEY_AI_MODEL = "model";
static const char *NVS_KEY_AI_STREAM = "ai_stream";

bool NVSManager::init() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    return (ret == ESP_OK);
}

bool NVSManager::getWiFiCred(std::string &ssid, std::string &pwd) {
    nvs_handle_t nvs;
    if (nvs_open("config", NVS_READONLY, &nvs) != ESP_OK) return false;
    char ssid_buf[33] = {0}, pwd_buf[65] = {0};
    size_t len = sizeof(ssid_buf);
    if (nvs_get_str(nvs, "ssid", ssid_buf, &len) != ESP_OK) { nvs_close(nvs); return false; }
    len = sizeof(pwd_buf);
    if (nvs_get_str(nvs, "pwd", pwd_buf, &len) != ESP_OK) { nvs_close(nvs); return false; }
    ssid = ssid_buf; pwd = pwd_buf;
    nvs_close(nvs);
    return true;
}

void NVSManager::setWiFiCred(const std::string &ssid, const std::string &pwd) {
    nvs_handle_t nvs;
    if (nvs_open("config", NVS_READWRITE, &nvs) == ESP_OK) {
        nvs_set_str(nvs, "ssid", ssid.c_str());
        nvs_set_str(nvs, "pwd", pwd.c_str());
        nvs_commit(nvs);
        nvs_close(nvs);
        ESP_LOGI(TAG, "WiFi saved");
    }
}

void NVSManager::clearWiFiCred() {
    nvs_handle_t nvs;
    if (nvs_open("config", NVS_READWRITE, &nvs) == ESP_OK) {
        nvs_erase_key(nvs, "ssid");
        nvs_erase_key(nvs, "pwd");
        nvs_commit(nvs);
        nvs_close(nvs);
        ESP_LOGI(TAG, "WiFi cleared");
    }
}

bool NVSManager::getAIConfig(std::string &url, std::string &apiKey, std::string &model) {
    nvs_handle_t nvs;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs) != ESP_OK) return false;

    char url_buf[192] = {0};
    char key_buf[128] = {0};
    char model_buf[96] = {0};

    size_t len = sizeof(url_buf);
    esp_err_t ret = nvs_get_str(nvs, NVS_KEY_AI_URL, url_buf, &len);
    if (ret != ESP_OK) {
        strncpy(url_buf, AI_DEFAULT_URL, sizeof(url_buf) - 1);
        url_buf[sizeof(url_buf) - 1] = '\0';
    }

    len = sizeof(key_buf);
    ret = nvs_get_str(nvs, NVS_KEY_AI_API_KEY, key_buf, &len);
    if (ret != ESP_OK || strlen(key_buf) == 0) {
        nvs_close(nvs);
        return false;
    }

    len = sizeof(model_buf);
    ret = nvs_get_str(nvs, NVS_KEY_AI_MODEL, model_buf, &len);
    if (ret != ESP_OK || strlen(model_buf) == 0) {
        strncpy(model_buf, "Qwen/Qwen2.5-7B-Instruct", sizeof(model_buf) - 1);
        model_buf[sizeof(model_buf) - 1] = '\0';
    }

    url = url_buf;
    apiKey = key_buf;
    model = model_buf;
    nvs_close(nvs);
    return true;
}

void NVSManager::setAIConfig(const std::string &url, const std::string &apiKey, const std::string &model) {
    nvs_handle_t nvs;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs) == ESP_OK) {
        const char *savedUrl = url.empty() ? AI_DEFAULT_URL : url.c_str();
        nvs_set_str(nvs, NVS_KEY_AI_URL, savedUrl);
        nvs_set_str(nvs, NVS_KEY_AI_API_KEY, apiKey.c_str());
        nvs_set_str(nvs, NVS_KEY_AI_MODEL, model.empty() ? "Qwen/Qwen2.5-7B-Instruct" : model.c_str());
        nvs_commit(nvs);
        nvs_close(nvs);
        ESP_LOGI(TAG, "AI config saved");
    }
}

bool NVSManager::getAIStreamEnabled() {
    nvs_handle_t nvs;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs) != ESP_OK) {
        return true;
    }
    uint8_t enabled = 1;
    const esp_err_t ret = nvs_get_u8(nvs, NVS_KEY_AI_STREAM, &enabled);
    nvs_close(nvs);
    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs) == ESP_OK) {
            nvs_set_u8(nvs, NVS_KEY_AI_STREAM, 1);
            nvs_commit(nvs);
            nvs_close(nvs);
            ESP_LOGI(TAG, "AI stream default saved: on");
        }
        return true;
    }
    return ret == ESP_OK ? enabled != 0 : true;
}

void NVSManager::setAIStreamEnabled(bool enabled) {
    nvs_handle_t nvs;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs) == ESP_OK) {
        nvs_set_u8(nvs, NVS_KEY_AI_STREAM, enabled ? 1 : 0);
        nvs_commit(nvs);
        nvs_close(nvs);
        ESP_LOGI(TAG, "AI stream mode saved: %s", enabled ? "on" : "off");
    }
}

void NVSManager::clearAIConfig() {
    nvs_handle_t nvs;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs) == ESP_OK) {
        nvs_erase_key(nvs, NVS_KEY_AI_URL);
        nvs_erase_key(nvs, NVS_KEY_AI_API_KEY);
        nvs_erase_key(nvs, NVS_KEY_AI_MODEL);
        nvs_erase_key(nvs, NVS_KEY_AI_STREAM);
        nvs_commit(nvs);
        nvs_close(nvs);
        ESP_LOGI(TAG, "AI config cleared");
    }
}
