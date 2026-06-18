idf_component_register(
        SRCS
        "app_main.cpp"
        "App.cpp"
        "NVSManager.cpp"
        "GPIOManager.cpp"
        "WiFiManager.cpp"
        "WebServerManager.cpp"
        "AIController.cpp"
        REQUIRES cjson
        PRIV_REQUIRES esp_https_server esp_wifi nvs_flash esp_eth esp_driver_uart esp_driver_gpio esp_http_client
        INCLUDE_DIRS "."
        EMBED_FILES "html/root.html" "html/chat.html"
)
