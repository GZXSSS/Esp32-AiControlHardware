//
// Created by GZX on 2026/6/18.
//
#pragma once
#include "NVSManager.hpp"
#include "GPIOManager.hpp"
#include "WiFiManager.hpp"
#include "WebServerManager.hpp"
#include "AIController.hpp"
#include <memory>

class App {
public:
    void run();

private:
    NVSManager nvs_;
    GPIOManager gpio_;
    WiFiManager wifi_{nvs_};
    std::unique_ptr<AIController> ai_;
    WebServerManager webServer_;

    
    void processUART();
    void startAPWebServer();
    void startSTAWebServer();
    void reloadAIController();
    static void initUART();
};