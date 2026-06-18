//
// Created by GZX on 2026/6/18.
//
#pragma once
#include <string>
#include <functional>
#define AI_API_URL "https://api.siliconflow.cn/v1/chat/completions"

class AIController {
public:
    using ResponseCallback = std::function<void(const std::string&)>;

    AIController(std::string url, std::string apiKey, std::string model);

    // 同步调用（阻塞当前任务，建议在独立任务中调用）
    void sendQuery(const std::string &userQuery, const ResponseCallback &callback);

private:
    std::string url_;
    std::string apiKey_;
    std::string model_;
};