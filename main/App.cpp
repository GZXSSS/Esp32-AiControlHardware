//
// Created by GZX on 2026/6/18.
//
#include "App.hpp"
#include "esp_log.h"
#include "driver/uart.h"
#include <cstring>
#include <initializer_list>
#include <string>
#include <algorithm>

#include "cJSON.h"

static const char *TAG = "App";

namespace {

std::string urlDecode(const char *data, size_t len) {
    std::string out;
    out.reserve(len);
    for (size_t i = 0; i < len; ++i) {
        if (data[i] == '+') {
            out.push_back(' ');
        } else if (data[i] == '%' && i + 2 < len) {
            auto hex = [](char c) -> int {
                if (c >= '0' && c <= '9') return c - '0';
                if (c >= 'a' && c <= 'f') return c - 'a' + 10;
                if (c >= 'A' && c <= 'F') return c - 'A' + 10;
                return -1;
            };
            int hi = hex(data[i + 1]);
            int lo = hex(data[i + 2]);
            if (hi >= 0 && lo >= 0) {
                out.push_back(static_cast<char>((hi << 4) | lo));
                i += 2;
            } else {
                out.push_back(data[i]);
            }
        } else {
            out.push_back(data[i]);
        }
    }
    return out;
}

bool getFormField(const std::string &body, const char *key, std::string &value) {
    const std::string prefix = std::string(key) + "=";
    const size_t pos = body.find(prefix);
    if (pos == std::string::npos) return false;
    const size_t start = pos + prefix.size();
    const size_t end = body.find('&', start);
    const std::string raw = body.substr(start, end == std::string::npos ? std::string::npos : end - start);
    value = urlDecode(raw.c_str(), raw.size());
    return true;
}

bool containsAny(const std::string &text, std::initializer_list<const char *> needles) {
    return std::any_of(needles.begin(), needles.end(), [&](const char *needle) {
        return text.find(needle) != std::string::npos;
    });
}

enum class LocalCommand { None, On, Off, Flow };

LocalCommand detectLocalCommand(const std::string &query) {
    if (containsAny(query, {"开灯", "打开灯", "开一下灯", "turn on", "light on", "on"})) {
        return LocalCommand::On;
    }
    if (containsAny(query, {"关灯", "关闭灯", "turn off", "light off", "off"})) {
        return LocalCommand::Off;
    }
    if (containsAny(query, {"流水", "跑马灯", "流光", "flow", "breath"})) {
        return LocalCommand::Flow;
    }
    return LocalCommand::None;
}

} // namespace

void App::run() {
    // 1. NVS
    if (!nvs_.init()) { ESP_LOGE(TAG, "NVS fail"); return; }

    // 2. GPIO
    gpio_.init();

    // 3. UART
    initUART();

    // 4. WiFi
    wifi_.init();
    bool staOk = wifi_.startSTA();
    if (staOk) {
        reloadAIController();
        startSTAWebServer();
    } else {
        wifi_.startAP();
        startAPWebServer();
    }

    // 5. Main loop
    while (true) {
        if (gpio_.isTriggered()) {
            ESP_LOGI(TAG, "Trigger reset");
            nvs_.clearWiFiCred();
            nvs_.clearAIConfig();
            reloadAIController();
            gpio_.stopFlow();
            gpio_.setD2(false);
            vTaskDelay(pdMS_TO_TICKS(500));
            esp_restart();
        }
        processUART();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void App::initUART() {
    uart_config_t uart_config = {};
    uart_config.baud_rate = 115200;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config.rx_flow_ctrl_thresh = 122;

    uart_param_config(UART_NUM_0, &uart_config);
    uart_driver_install(UART_NUM_0, 1024, 0, 0, nullptr, 0);
}

void App::processUART() {
    uint8_t buf[64];
    int len = uart_read_bytes(UART_NUM_0, buf, sizeof(buf)-1, pdMS_TO_TICKS(10));
    if (len > 0) {
        buf[len] = '\0';
        char *p = strchr((char*)buf, '\n'); if (p) *p = '\0';
        p = strchr((char*)buf, '\r'); if (p) *p = '\0';
        if (strlen((char*)buf) == 0) return;
        ESP_LOGI(TAG, "UART: %s", buf);
        if (strcmp((char*)buf, "on") == 0) gpio_.setD2(true);
        else if (strcmp((char*)buf, "off") == 0) gpio_.setD2(false);
        else if (strcmp((char*)buf, "liu") == 0) gpio_.startFlow();
        else ESP_LOGW(TAG, "Unknown");
    }
}

// ---------- AP配网Web ----------
void App::startAPWebServer() {
    std::vector<WebServerManager::Route> routes;

    routes.push_back({"/", HTTP_GET, [](httpd_req_t *req) -> esp_err_t {
        extern const uint8_t root_html_start[] asm("_binary_root_html_start");
        extern const uint8_t root_html_end[]   asm("_binary_root_html_end");
        size_t len = root_html_end - root_html_start;
        httpd_resp_set_type(req, "text/html");
        httpd_resp_send(req, reinterpret_cast<const char *>(root_html_start), static_cast<ssize_t>(len));
        return ESP_OK;
    }});

    routes.push_back({"/scan", HTTP_GET, [this](httpd_req_t *req) -> esp_err_t {
        auto list = wifi_.scanAPs();
        cJSON *root = cJSON_CreateArray();
        for (auto &s : list) cJSON_AddItemToArray(root, cJSON_CreateString(s.c_str()));
        char *json = cJSON_PrintUnformatted(root);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, json, static_cast<ssize_t>(strlen(json)));
        free(json); cJSON_Delete(root);
        return ESP_OK;
    }});

    routes.push_back({"/save", HTTP_GET, [this](httpd_req_t *req) -> esp_err_t {
        char query[128] = {0};
        if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No query");
            return ESP_FAIL;
        }
        char ssid[64] = {0}, pwd[64] = {0};
        httpd_query_key_value(query, "ssid", ssid, sizeof(ssid));
        httpd_query_key_value(query, "pwd", pwd, sizeof(pwd));
        if (strlen(ssid)==0 || strlen(pwd)==0) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Empty");
            return ESP_FAIL;
        }
        nvs_.setWiFiCred(ssid, pwd);
        httpd_resp_send(req, "Saved, rebooting...", HTTPD_RESP_USE_STRLEN);
        vTaskDelay(pdMS_TO_TICKS(500));
        esp_restart();
    }});

    webServer_.start(routes);
    wifi_.setWebServer(webServer_.getHandle());
}

// ---------- STA控制Web ----------
void App::startSTAWebServer() {
    std::vector<WebServerManager::Route> routes;

    // 根页面：AI 聊天界面
    routes.push_back({"/", HTTP_GET, [](httpd_req_t *req) -> esp_err_t {
        extern const uint8_t chat_html_start[] asm("_binary_chat_html_start");
        extern const uint8_t chat_html_end[]   asm("_binary_chat_html_end");
        size_t len = chat_html_end - chat_html_start;
        httpd_resp_set_type(req, "text/html");
        httpd_resp_send(req, reinterpret_cast<const char *>(chat_html_start), static_cast<ssize_t>(len));
        return ESP_OK;
    }});

    // 手动控制：开灯
    routes.push_back({"/on", HTTP_GET, [this](httpd_req_t *req) -> esp_err_t {
        gpio_.setD2(true);
        httpd_resp_send(req, "D2 ON", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }});

    // 手动控制：关灯
    routes.push_back({"/off", HTTP_GET, [this](httpd_req_t *req) -> esp_err_t {
        gpio_.setD2(false);
        httpd_resp_send(req, "D2 OFF", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }});

    // 流水灯模式
    routes.push_back({"/liu", HTTP_GET, [this](httpd_req_t *req) -> esp_err_t {
        gpio_.startFlow();
        httpd_resp_send(req, "Flow started", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }});

    // 获取当前 AI 配置（返回 JSON）
    routes.push_back({"/get_ai_config", HTTP_GET, [this](httpd_req_t *req) -> esp_err_t {
        std::string url, apiKey, model;
        bool hasConfig = nvs_.getAIConfig(url, apiKey, model);
        cJSON *root = cJSON_CreateObject();
        cJSON_AddBoolToObject(root, "configured", hasConfig);
        cJSON_AddStringToObject(root, "url", url.empty() ? AI_DEFAULT_URL : url.c_str());
        if (hasConfig) {
            cJSON_AddStringToObject(root, "api_key", apiKey.c_str());
            cJSON_AddStringToObject(root, "model", model.c_str());
        }
        char *json = cJSON_PrintUnformatted(root);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, json, static_cast<ssize_t>(strlen(json)));
        free(json);
        cJSON_Delete(root);
        return ESP_OK;
    }});

    routes.push_back({"/set_ai_config", HTTP_POST, [this](httpd_req_t *req) -> esp_err_t {
        char buffer[512] = {0};
        int ret = httpd_req_recv(req, buffer, sizeof(buffer) - 1);
        if (ret <= 0) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No data");
            return ESP_FAIL;
        }
        buffer[ret] = '\0';  // 确保字符串结束

        std::string apiKey, model, url;
        getFormField(buffer, "api_key", apiKey);
        getFormField(buffer, "model", model);
        getFormField(buffer, "url", url);

        if (apiKey.empty() || model.empty()) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing api_key or model");
            return ESP_FAIL;
        }
        nvs_.setAIConfig(url, apiKey, model);
        reloadAIController();
        httpd_resp_send(req, "Config saved and applied", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }});

    routes.push_back({"/clear_ai_config", HTTP_POST, [this](httpd_req_t *req) -> esp_err_t {
        nvs_.clearAIConfig();
        reloadAIController();
        httpd_resp_send(req, "AI config cleared", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }});

    // AI 聊天接口（通过 GET 参数 q=问题）
    routes.push_back({"/chat", HTTP_GET, [this](httpd_req_t *req) -> esp_err_t {
        char query[192] = {0};
        if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No q");
            return ESP_FAIL;
        }
        char q_raw[160] = {0};
        if (httpd_query_key_value(query, "q", q_raw, sizeof(q_raw)) != ESP_OK || strlen(q_raw) == 0) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Empty");
            return ESP_FAIL;
        }

        std::string q = q_raw;
        switch (detectLocalCommand(q)) {
            case LocalCommand::On:
                gpio_.setD2(true);
                httpd_resp_set_type(req, "application/json; charset=utf-8");
                httpd_resp_sendstr(req, "{\"type\":\"control\",\"action\":\"on\",\"reply\":\"已开灯\"}");
                return ESP_OK;
            case LocalCommand::Off:
                gpio_.setD2(false);
                httpd_resp_set_type(req, "application/json; charset=utf-8");
                httpd_resp_sendstr(req, "{\"type\":\"control\",\"action\":\"off\",\"reply\":\"已关灯\"}");
                return ESP_OK;
            case LocalCommand::Flow:
                gpio_.startFlow();
                httpd_resp_set_type(req, "application/json; charset=utf-8");
                httpd_resp_sendstr(req, "{\"type\":\"control\",\"action\":\"flow\",\"reply\":\"已启动流水灯\"}");
                return ESP_OK;
            case LocalCommand::None:
                break;
        }

        if (!ai_) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "AI not configured");
            return ESP_FAIL;
        }

        std::string reply;
        ai_->sendQuery(q, [&reply](const std::string &resp) { reply = resp; });

        if (reply.empty()) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "AI error");
            return ESP_FAIL;
        }

        if (containsAny(reply, {"D2 turned ON", "打开灯", "开灯", "turn on", "turn on the light"})) {
            gpio_.setD2(true);
        } else if (containsAny(reply, {"D2 turned OFF", "关闭灯", "关灯", "turn off", "turn off the light"})) {
            gpio_.setD2(false);
        } else if (containsAny(reply, {"Flow started", "流水", "flow"})) {
            gpio_.startFlow();
        }

        cJSON *root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "type", "ai");
        cJSON_AddStringToObject(root, "reply", reply.c_str());
        char *json = cJSON_PrintUnformatted(root);
        httpd_resp_set_type(req, "application/json; charset=utf-8");
        httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
        free(json);
        cJSON_Delete(root);
        return ESP_OK;
    }});

    webServer_.start(routes);

    wifi_.setWebServer(webServer_.getHandle());
}

void App::reloadAIController() {
    // 先释放旧的实例（如果存在）
    ai_.reset();

    std::string url, apiKey, model;
    if (nvs_.getAIConfig(url, apiKey, model)) {
        ai_ = std::make_unique<AIController>(url, apiKey, model);
        ESP_LOGI(TAG, "AI controller reloaded with new config: %s", url.c_str());
    } else {
        ESP_LOGW(TAG, "No AI config found, AI disabled");
    }
}