#include "wss_client.h"

#define TAG "wss-client"

// 组包 WebSocket 文本帧，返回帧长度
size_t wss_client_build_frame(const char *msg, uint8_t *frame_buf, size_t buf_size) {
    size_t msg_len = strlen(msg);
    if (buf_size < msg_len + 6) return 0; // 缓冲区不足
    frame_buf[0] = 0x81; // FIN + text frame
    frame_buf[1] = 0x80 | (msg_len & 0x7F); // MASK bit + payload len
    uint8_t mask[4];
    for (int i = 0; i < 4; ++i) mask[i] = esp_random() & 0xFF;
    memcpy(&frame_buf[2], mask, 4);
    for (size_t i = 0; i < msg_len; ++i) {
        frame_buf[6 + i] = msg[i] ^ mask[i % 4];
    }
    return msg_len + 6;
}

static void wss_client_task(void *param) {
    const wss_client_config_t *config = (const wss_client_config_t *)param;
    // 这里只做 ws:// 的简单演示，不支持 wss://
    // 实际项目建议用更完整的库或完善此实现
    struct addrinfo hints = {0}, *res;
    // 解析 ws://host[:port][/path]
    char host[128] = {0};
    char path[128] = "/";
    int port = 80;
    const char *p = config->uri + strlen("ws://");
    const char *slash = strchr(p, '/');
    const char *colon = strchr(p, ':');
    if (colon && (!slash || colon < slash)) {
        strncpy(host, p, colon - p);
        port = atoi(colon + 1);
        if (slash) strncpy(path, slash, sizeof(path) - 1);
    } else {
        if (slash) {
            strncpy(host, p, slash - p);
            strncpy(path, slash, sizeof(path) - 1);
        } else {
            strncpy(host, p, sizeof(host) - 1);
        }
    }
    ESP_LOGI(TAG, "Connecting to %s:%d%s", host, port, path);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host, NULL, &hints, &res) != 0) {
        ESP_LOGE(TAG, "DNS lookup failed");
        vTaskDelete(NULL);
        return;
    }
    struct sockaddr_in *addr = (struct sockaddr_in *)res->ai_addr;
    addr->sin_port = htons(port);
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        ESP_LOGE(TAG, "Socket create failed");
        freeaddrinfo(res);
        vTaskDelete(NULL);
        return;
    }
    if (connect(sock, (struct sockaddr *)addr, sizeof(struct sockaddr_in)) != 0) {
        ESP_LOGE(TAG, "Socket connect failed");
        close(sock);
        freeaddrinfo(res);
        vTaskDelete(NULL);
        return;
    }
    freeaddrinfo(res);
    // 发送 WebSocket 握手
    char req[512];
    snprintf(req, sizeof(req),
        "GET %s HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "\r\n",
        path, host, port);
    send(sock, req, strlen(req), 0);
    char resp[512];
    int len = recv(sock, resp, sizeof(resp)-1, 0);
    if (len <= 0) {
        ESP_LOGE(TAG, "Handshake failed");
        close(sock);
        vTaskDelete(NULL);
        return;
    }
    resp[len] = 0;
    ESP_LOGI(TAG, "Handshake response: %s", resp);
    // 简单循环收消息和发消息
    while (1) {
        const char *msg = "Hello from ESP32";
        uint8_t frame[128] = {0};
        size_t frame_len = wss_client_build_frame(msg, frame, sizeof(frame));
        if (frame_len > 0) {
            send(sock, frame, frame_len, 0);
            ESP_LOGI(TAG, "Sent: %s", msg);
        }
        // 接收服务器返回
        uint8_t hdr[2];
        int r = recv(sock, hdr, 2, 0);
        if (r <= 0) break;
        int payload_len = hdr[1] & 0x7F;
        char recv_msg[128] = {0};
        if (payload_len > 0 && payload_len < sizeof(recv_msg)) {
            r = recv(sock, recv_msg, payload_len, 0);
            if (r > 0 && config->on_message) {
                config->on_message(recv_msg, r);
            }
        }
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
    close(sock);
    vTaskDelete(NULL);
}

void wss_client_start(const wss_client_config_t *config) {
    xTaskCreate(wss_client_task, "wss_client", 4096, (void *)config, 5, NULL);
}

