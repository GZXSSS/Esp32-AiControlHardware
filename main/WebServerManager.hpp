#pragma once
#include <vector>
#include <string>
#include <functional>
#include "esp_http_server.h"

class WebServerManager {
public:
    using Handler = std::function<esp_err_t(httpd_req_t*)>;

    struct Route {
        std::string uri;
        httpd_method_t method;  // 添加 method 字段
        Handler handler;
    };

    WebServerManager() : server_(nullptr) {}
    ~WebServerManager() { stop(); }

    bool start(const std::vector<Route>& routes);
    void stop();
    httpd_handle_t getHandle() const { return server_; }

private:
    httpd_handle_t server_;
    std::vector<Handler> handlers_; // 持有 lambda 以保证生命周期

    static esp_err_t staticHandler(httpd_req_t *req);
};