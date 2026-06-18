//
// Created by GZX on 2026/6/18.
//
#include "AIController.hpp"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "cJSON.h"
#include <cstring>
#include <string>
#include <utility>
#include <cstdlib>


static const char *TAG = "AI";

static esp_err_t ai_http_event_handler(esp_http_client_event_t *evt) {
    if (evt->event_id == HTTP_EVENT_ON_DATA && evt->user_data && evt->data && evt->data_len > 0) {
        auto *body = static_cast<std::string *>(evt->user_data);
        body->append(static_cast<const char *>(evt->data), evt->data_len);
    }
    return ESP_OK;
}

static void callbackWithError(const AIController::ResponseCallback &callback, const char *message) {
    if (callback) {
        callback(message ? message : "");
    }
}

AIController::AIController(std::string url, std::string apiKey, std::string model)
    : url_(url.empty() ? AI_API_URL : std::move(url)), apiKey_(std::move(apiKey)), model_(std::move(model)) {}

void AIController::sendQuery(const std::string &userQuery, const ResponseCallback &callback) {
    cJSON *root = cJSON_CreateObject();
    cJSON *messages = cJSON_CreateArray();
    cJSON *message = cJSON_CreateObject();
    if (!root || !messages || !message) {
        ESP_LOGE(TAG, "Failed to build request JSON");
        if (message) cJSON_Delete(message);
        if (messages) cJSON_Delete(messages);
        if (root) cJSON_Delete(root);
        if (callback) callback("");
        return;
    }

    cJSON_AddStringToObject(root, "model", model_.c_str());
    cJSON_AddStringToObject(message, "role", "user");
    cJSON_AddStringToObject(message, "content", userQuery.c_str());
    cJSON_AddItemToArray(messages, message);
    cJSON_AddItemToObject(root, "messages", messages);

    char *post_data = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!post_data) {
        ESP_LOGE(TAG, "Failed to print request JSON");
        if (callback) callback("");
        return;
    }

    esp_http_client_config_t config = {};
    config.url = url_.c_str();
    config.method = HTTP_METHOD_POST;
    config.timeout_ms = 15000;
    config.transport_type = HTTP_TRANSPORT_OVER_SSL;
    config.crt_bundle_attach = esp_crt_bundle_attach;
    std::string body;
    config.event_handler = ai_http_event_handler;
    config.user_data = &body;
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to init HTTP client");
        free(post_data);
        callbackWithError(callback, "Failed to init HTTP client");
        return;
    }

    char auth_header[160];
    snprintf(auth_header, sizeof(auth_header), "Bearer %s", apiKey_.c_str());
    esp_http_client_set_header(client, "Authorization", auth_header);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        if (status == 200) {
            cJSON *resp = cJSON_Parse(body.c_str());
            if (resp) {
                cJSON *choices = cJSON_GetObjectItem(resp, "choices");
                cJSON *first = nullptr;
                cJSON *msg = nullptr;
                cJSON *content = nullptr;
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wanalyzer-null-dereference"
#endif
                if (choices == nullptr) {
                    ESP_LOGE(TAG, "Missing choices in AI response");
                } else if (cJSON_IsArray(choices) && cJSON_GetArraySize(choices) > 0) {
                    first = cJSON_GetArrayItem(choices, 0);
                    if (first != nullptr && cJSON_IsObject(first)) {
                        msg = cJSON_GetObjectItem(first, "message");
                        if (msg != nullptr && cJSON_IsObject(msg)) {
                            content = cJSON_GetObjectItem(msg, "content");
                        }
                    }
                }
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
                cJSON *error = cJSON_GetObjectItem(resp, "error");

                std::string reply;
                if (cJSON_IsString(content) && content->valuestring) {
                    reply = content->valuestring;
                } else if (cJSON_IsObject(error)) {
                    cJSON *errMsg = cJSON_GetObjectItem(error, "message");
                    if (cJSON_IsString(errMsg) && errMsg->valuestring) {
                        reply = errMsg->valuestring;
                    }
                }
                cJSON_Delete(resp);

                esp_http_client_cleanup(client);
                free(post_data);
                if (callback) callback(reply.empty() ? std::string("AI returned an empty response") : reply);
                return;
            }
            ESP_LOGE(TAG, "Failed to parse AI response: %s", body.c_str());
        } else {
            ESP_LOGE(TAG, "HTTP status %d, body: %s", status, body.c_str());
        }
    } else {
        ESP_LOGE(TAG, "Perform failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
    free(post_data);
    if (body.empty()) {
        callbackWithError(callback, esp_err_to_name(err));
    } else {
        callbackWithError(callback, body.c_str());
    }
}