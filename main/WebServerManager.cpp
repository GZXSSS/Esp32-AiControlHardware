#include "WebServerManager.hpp"
#include "esp_log.h"

static const char *TAG = "WebServer";

bool WebServerManager::start(const std::vector<Route>& routes) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    config.stack_size = 12288;
    if (httpd_start(&server_, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start server");
        return false;
    }

    handlers_.clear();
    handlers_.reserve(routes.size());

    for (const auto& route : routes) {
        handlers_.push_back(route.handler); // 拷贝
        httpd_uri_t http_uri = {
            .uri = route.uri.c_str(),
            .method = route.method,
            .handler = staticHandler,
            .user_ctx = &handlers_.back()
        };
        if (httpd_register_uri_handler(server_, &http_uri) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to register %s", route.uri.c_str());
        } else {
            ESP_LOGI(TAG, "Registered %s (%d)", route.uri.c_str(), route.method);
        }
    }
    return true;
}

void WebServerManager::stop() {
    if (server_) {
        httpd_stop(server_);
        server_ = nullptr;
    }
    handlers_.clear();
}

esp_err_t WebServerManager::staticHandler(httpd_req_t *req) {
    auto *handler = static_cast<Handler *>(req->user_ctx);
    if (handler) return (*handler)(req);
    httpd_resp_send_404(req);
    return ESP_FAIL;
}