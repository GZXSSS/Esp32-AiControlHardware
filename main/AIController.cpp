//
// Created by GZX on 2026/6/18.
//
#include "AIController.hpp"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "cJSON.h"
#include <algorithm>
#include <cstring>
#include <string>
#include <utility>
#include <cstdlib>


static const char *TAG = "AI";

namespace {

constexpr size_t AI_BODY_LIMIT_NON_STREAM = 64 * 1024;
constexpr size_t AI_BODY_LIMIT_STREAM = 4 * 1024;
constexpr size_t AI_STREAM_REPLY_LIMIT = 8 * 1024;
constexpr size_t AI_SSE_BUFFER_LIMIT = 8 * 1024;

struct HttpEventContext {
    std::string body;
    bool streamMode = false;
    std::string sseBuffer;
    std::string streamReply;
    std::string streamError;
    bool streamReplyTruncated = false;
    AIController::StreamChunkCallback onChunk;
};

void appendWithLimit(std::string &target, const char *data, size_t len, size_t maxSize) {
    if (!data || len == 0 || target.size() >= maxSize) {
        return;
    }
    const size_t remaining = maxSize - target.size();
    target.append(data, len > remaining ? remaining : len);
}

char *buildPostData(const std::string &model, const std::string &userQuery, bool streamMode) {
    cJSON *root = cJSON_CreateObject();
    cJSON *messages = cJSON_CreateArray();
    cJSON *message = cJSON_CreateObject();
    if (!root || !messages || !message) {
        if (message) cJSON_Delete(message);
        if (messages) cJSON_Delete(messages);
        if (root) cJSON_Delete(root);
        return nullptr;
    }

    cJSON_AddStringToObject(root, "model", model.c_str());
    cJSON_AddStringToObject(message, "role", "user");
    cJSON_AddStringToObject(message, "content", userQuery.c_str());
    cJSON_AddItemToArray(messages, message);
    cJSON_AddItemToObject(root, "messages", messages);
    cJSON_AddBoolToObject(root, "stream", streamMode);

    char *postData = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return postData;
}

std::string parseNonStreamReply(const std::string &body) {
    cJSON *resp = cJSON_Parse(body.c_str());
    if (!resp) {
        return {};
    }

    cJSON *choices = cJSON_GetObjectItem(resp, "choices");
    cJSON *first = nullptr;
    cJSON *msg = nullptr;
    cJSON *content = nullptr;
    if (cJSON_IsArray(choices) && cJSON_GetArraySize(choices) > 0) {
        first = cJSON_GetArrayItem(choices, 0);
        if (first != nullptr && cJSON_IsObject(first)) {
            msg = cJSON_GetObjectItem(first, "message");
            if (msg != nullptr && cJSON_IsObject(msg)) {
                content = cJSON_GetObjectItem(msg, "content");
            }
        }
    }

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
    return reply;
}

void processSSELine(HttpEventContext &ctx, const std::string &line) {
    if (line.rfind("data:", 0) != 0) {
        return;
    }
    std::string payload = line.substr(5);
    payload.erase(payload.begin(), std::find_if(payload.begin(), payload.end(), [](unsigned char c) {
        return c != ' ' && c != '\t';
    }));
    if (payload.empty() || payload == "[DONE]") {
        return;
    }

    cJSON *resp = cJSON_Parse(payload.c_str());
    if (!resp) {
        return;
    }

    cJSON *error = cJSON_GetObjectItem(resp, "error");
    if (cJSON_IsObject(error)) {
        cJSON *errMsg = cJSON_GetObjectItem(error, "message");
        if (cJSON_IsString(errMsg) && errMsg->valuestring) {
            ctx.streamError = errMsg->valuestring;
        }
    }

    cJSON *choices = cJSON_GetObjectItem(resp, "choices");
    if (cJSON_IsArray(choices) && cJSON_GetArraySize(choices) > 0) {
        cJSON *first = cJSON_GetArrayItem(choices, 0);
        cJSON *delta = first ? cJSON_GetObjectItem(first, "delta") : nullptr;
        cJSON *content = delta ? cJSON_GetObjectItem(delta, "content") : nullptr;
        if (cJSON_IsString(content) && content->valuestring && content->valuestring[0] != '\0') {
            if (!ctx.streamReplyTruncated) {
                const size_t chunkLen = strlen(content->valuestring);
                if (ctx.streamReply.size() + chunkLen <= AI_STREAM_REPLY_LIMIT) {
                    ctx.streamReply.append(content->valuestring, chunkLen);
                } else {
                    const size_t remaining = AI_STREAM_REPLY_LIMIT - ctx.streamReply.size();
                    if (remaining > 0) {
                        ctx.streamReply.append(content->valuestring, remaining);
                    }
                    ctx.streamReplyTruncated = true;
                }
            }
            if (ctx.onChunk) {
                ctx.onChunk(content->valuestring);
            }
        }
    }
    cJSON_Delete(resp);
}

} // namespace

static esp_err_t ai_http_event_handler(esp_http_client_event_t *evt) {
    if (evt->event_id == HTTP_EVENT_ON_DATA && evt->user_data && evt->data && evt->data_len > 0) {
        auto *ctx = static_cast<HttpEventContext *>(evt->user_data);
        const size_t bodyLimit = ctx->streamMode ? AI_BODY_LIMIT_STREAM : AI_BODY_LIMIT_NON_STREAM;
        appendWithLimit(ctx->body, static_cast<const char *>(evt->data), evt->data_len, bodyLimit);

        if (ctx->streamMode) {
            appendWithLimit(ctx->sseBuffer, static_cast<const char *>(evt->data), evt->data_len, AI_SSE_BUFFER_LIMIT);
            size_t lineEnd = 0;
            while ((lineEnd = ctx->sseBuffer.find('\n')) != std::string::npos) {
                std::string line = ctx->sseBuffer.substr(0, lineEnd);
                ctx->sseBuffer.erase(0, lineEnd + 1);
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }
                processSSELine(*ctx, line);
            }
        }
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
    char *post_data = buildPostData(model_, userQuery, false);
    if (!post_data) {
        ESP_LOGE(TAG, "Failed to print request JSON");
        callbackWithError(callback, "Failed to build request JSON");
        return;
    }

    esp_http_client_config_t config = {};
    config.url = url_.c_str();
    config.method = HTTP_METHOD_POST;
    config.timeout_ms = 15000;
    config.transport_type = HTTP_TRANSPORT_OVER_SSL;
    config.crt_bundle_attach = esp_crt_bundle_attach;
    HttpEventContext ctx;
    config.event_handler = ai_http_event_handler;
    config.user_data = &ctx;
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
            std::string reply = parseNonStreamReply(ctx.body);
            if (!reply.empty()) {
                esp_http_client_cleanup(client);
                free(post_data);
                if (callback) callback(reply);
                return;
            }
            ESP_LOGE(TAG, "Failed to parse AI response: %s", ctx.body.c_str());
        } else {
            ESP_LOGE(TAG, "HTTP status %d, body: %s", status, ctx.body.c_str());
        }
    } else {
        ESP_LOGE(TAG, "Perform failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
    free(post_data);
    if (ctx.body.empty()) {
        callbackWithError(callback, esp_err_to_name(err));
    } else {
        callbackWithError(callback, ctx.body.c_str());
    }
}

void AIController::sendQueryStream(const std::string &userQuery,
                                   const StreamChunkCallback &onChunk,
                                   const ResponseCallback &onDone) {
    char *post_data = buildPostData(model_, userQuery, true);
    if (!post_data) {
        ESP_LOGE(TAG, "Failed to print request JSON");
        callbackWithError(onDone, "Failed to build request JSON");
        return;
    }

    esp_http_client_config_t config = {};
    config.url = url_.c_str();
    config.method = HTTP_METHOD_POST;
    config.timeout_ms = 45000;
    config.transport_type = HTTP_TRANSPORT_OVER_SSL;
    config.crt_bundle_attach = esp_crt_bundle_attach;

    HttpEventContext ctx;
    ctx.streamMode = true;
    ctx.onChunk = onChunk;
    config.event_handler = ai_http_event_handler;
    config.user_data = &ctx;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to init HTTP client");
        free(post_data);
        callbackWithError(onDone, "Failed to init HTTP client");
        return;
    }

    char auth_header[160];
    snprintf(auth_header, sizeof(auth_header), "Bearer %s", apiKey_.c_str());
    esp_http_client_set_header(client, "Authorization", auth_header);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Accept", "text/event-stream");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    const esp_err_t err = esp_http_client_perform(client);
    const int status = (err == ESP_OK) ? esp_http_client_get_status_code(client) : 0;

    if (!ctx.sseBuffer.empty()) {
        processSSELine(ctx, ctx.sseBuffer);
        ctx.sseBuffer.clear();
    }

    esp_http_client_cleanup(client);
    free(post_data);

    if (err != ESP_OK) {
        callbackWithError(onDone, esp_err_to_name(err));
        return;
    }
    if (status != 200) {
        callbackWithError(onDone, ctx.body.empty() ? "HTTP error" : ctx.body.c_str());
        return;
    }
    if (!ctx.streamError.empty() && ctx.streamReply.empty()) {
        callbackWithError(onDone, ctx.streamError.c_str());
        return;
    }
    if (onDone) {
        onDone(ctx.streamReply.empty() ? std::string("AI returned an empty response") : ctx.streamReply);
    }
}