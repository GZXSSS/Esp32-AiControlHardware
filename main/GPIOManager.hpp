//
// Created by GZX on 2026/6/18.
//
#pragma once
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define D2_PIN        GPIO_NUM_2
#define TRIGGER_PIN   GPIO_NUM_13

class GPIOManager {
public:
    void init();

    void setD2(bool on);
    void startFlow();
    void stopFlow();
    bool isTriggered();

private:
    TaskHandle_t flowTaskHandle = nullptr;
    bool flowEnabled = false;
    bool d2State = false;

    static void flowTask(void *arg);
};