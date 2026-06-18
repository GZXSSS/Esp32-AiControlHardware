//
// Created by GZX on 2026/6/18.
//
#pragma once
#include <string>
#include <vector>
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_netif.h"

class NVSManager;

class WiFiManager {
public:
    WiFiManager(NVSManager &nvs);
    ~WiFiManager();

    bool init();
    bool startSTA();
    void startAP();
    bool isAPMode() const { return is_ap_mode_; }
    std::vector<std::string> scanAPs();
    void setWebServer(httpd_handle_t server) { server_ = server; }

private:
    NVSManager &nvs_;
    EventGroupHandle_t wifi_event_group_;
    esp_netif_t *sta_netif_;
    esp_netif_t *ap_netif_;
    httpd_handle_t server_;
    bool is_ap_mode_;

    static void eventHandler(void *arg, esp_event_base_t event_base,
                             int32_t event_id, void *event_data);
};