//
// Created by GZX on 2026/6/18.
//
#include "WiFiManager.hpp"
#include "NVSManager.hpp"
#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/event_groups.h"

#define AP_SSID       "ESP32_Setup"
#define AP_PASSWORD   "gzx123456"
#define AP_MAX_CONN   4
#define WIFI_CONNECTED_BIT BIT0

static const char *TAG = "WiFi";

WiFiManager::WiFiManager(NVSManager &nvs)
    : nvs_(nvs), wifi_event_group_(nullptr),
      sta_netif_(nullptr), ap_netif_(nullptr), server_(nullptr), is_ap_mode_(false) {}

WiFiManager::~WiFiManager() {
    if (wifi_event_group_) vEventGroupDelete(wifi_event_group_);
}

bool WiFiManager::init() {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_event_group_ = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, eventHandler, this));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, eventHandler, this));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    return true;
}

bool WiFiManager::startSTA() {
    std::string ssid, pwd;
    if (!nvs_.getWiFiCred(ssid, pwd)) {
        ESP_LOGI(TAG, "No saved credentials");
        return false;
    }
    ESP_LOGI(TAG, "Connecting to %s", ssid.c_str());
    if (sta_netif_ == nullptr) sta_netif_ = esp_netif_create_default_wifi_sta();

    wifi_config_t wifi_config = {};
    strncpy((char *)wifi_config.sta.ssid, ssid.c_str(), sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, pwd.c_str(), sizeof(wifi_config.sta.password));
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    EventBits_t bits = xEventGroupWaitBits(wifi_event_group_, WIFI_CONNECTED_BIT,
                                           pdFALSE, pdTRUE, pdMS_TO_TICKS(15000));
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "STA connected");
        is_ap_mode_ = false;
        return true;
    }
    ESP_LOGI(TAG, "STA connection failed");
    return false;
}

void WiFiManager::startAP() {
    ESP_LOGI(TAG, "Starting AP mode");
    esp_wifi_stop();
    if (sta_netif_) { esp_netif_destroy_default_wifi(sta_netif_); sta_netif_ = nullptr; }
    if (ap_netif_ == nullptr) ap_netif_ = esp_netif_create_default_wifi_ap();
    if (sta_netif_ == nullptr) sta_netif_ = esp_netif_create_default_wifi_sta();

    wifi_config_t ap_config = {};
    strncpy((char *)ap_config.ap.ssid, AP_SSID, sizeof(ap_config.ap.ssid));
    strncpy((char *)ap_config.ap.password, AP_PASSWORD, sizeof(ap_config.ap.password));
    ap_config.ap.ssid_len = strlen(AP_SSID);
    ap_config.ap.max_connection = AP_MAX_CONN;
    ap_config.ap.authmode = (strlen(AP_PASSWORD)==0) ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA_WPA2_PSK;
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    wifi_config_t sta_config = {};
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    is_ap_mode_ = true;
}

std::vector<std::string> WiFiManager::scanAPs() {
    wifi_scan_config_t scan_config = {};
    scan_config.show_hidden = true;
    scan_config.scan_type = WIFI_SCAN_TYPE_ACTIVE;
    scan_config.scan_time.active.min = 100;
    scan_config.scan_time.active.max = 300;
    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));

    uint16_t ap_num = 0;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_num));
    std::vector<std::string> ssids;
    if (ap_num > 0) {
        wifi_ap_record_t *records = (wifi_ap_record_t*)malloc(sizeof(wifi_ap_record_t)*ap_num);
        if (records) {
            ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_num, records));
            for (int i=0; i<ap_num; i++) ssids.push_back((const char*)records[i].ssid);
            free(records);
        }
    }
    return ssids;
}

void WiFiManager::eventHandler(void *arg, esp_event_base_t base, int32_t id, void *data) {
    WiFiManager *self = (WiFiManager*)arg;
    if (base == WIFI_EVENT) {
        switch (id) {
            case WIFI_EVENT_STA_START: esp_wifi_connect(); break;
            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGI(TAG, "Disconnected, reconnect");
                xEventGroupClearBits(self->wifi_event_group_, WIFI_CONNECTED_BIT);
                esp_wifi_connect();
                break;
            default: break;
        }
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *ev = (ip_event_got_ip_t*)data;
        ESP_LOGI(TAG, "Got IP " IPSTR, IP2STR(&ev->ip_info.ip));
        xEventGroupSetBits(self->wifi_event_group_, WIFI_CONNECTED_BIT);
    }
}