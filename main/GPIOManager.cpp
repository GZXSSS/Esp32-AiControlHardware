//
// Created by GZX on 2026/6/18.
//
#include "GPIOManager.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "GPIO";

void GPIOManager::init() {
    gpio_config_t io_out = {
        .pin_bit_mask = (1ULL << D2_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_out);
    gpio_set_level(D2_PIN, 0);

    gpio_config_t io_in = {
        .pin_bit_mask = (1ULL << TRIGGER_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_in);
}

void GPIOManager::setD2(bool on) {
    stopFlow();
    gpio_set_level(D2_PIN, on ? 1 : 0);
    d2State = on;
    ESP_LOGI(TAG, "D2 set to %d", on);
}

void GPIOManager::startFlow() {
    if (flowTaskHandle == nullptr) {
        xTaskCreate(flowTask, "flow_led", 2048, this, 5, &flowTaskHandle);
    }
    flowEnabled = true;
    d2State = true;
    gpio_set_level(D2_PIN, 1);
    ESP_LOGI(TAG, "Flow started");
}

void GPIOManager::stopFlow() {
    flowEnabled = false;
}

bool GPIOManager::isTriggered() {
    static bool last_level = true;
    bool current = gpio_get_level(TRIGGER_PIN);
    if (last_level == true && current == false) {
        vTaskDelay(pdMS_TO_TICKS(50));
        if (gpio_get_level(TRIGGER_PIN) == false) {
            last_level = current;
            return true;
        }
    }
    last_level = current;
    return false;
}

void GPIOManager::flowTask(void *arg) {
    GPIOManager *self = (GPIOManager*)arg;
    while (true) {
        if (self->flowEnabled) {
            self->d2State = !self->d2State;
            gpio_set_level(D2_PIN, self->d2State ? 1 : 0);
            vTaskDelay(pdMS_TO_TICKS(300));
        } else {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}