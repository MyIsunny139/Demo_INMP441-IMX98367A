#include "wss_client.h"
#include <errno.h>

#define TAG "wss_client"

//? WebSocket全局socket句柄，用于发送和接收任务共享
static int g_websocket_sock = -1;
//? WebSocket配置，用于回调函数
static const wss_client_config_t *g_config = NULL;

//? 组包 WebSocket 文本帧，返回帧长度
static size_t build_websocket_frame(const char *msg, uint8_t *frame_buf, size_t buf_size) 
{
    size_t msg_len = strlen(msg);
    if (buf_size < msg_len + 6) 
    {
        return 0;   // 缓冲区不足
    }
    
    frame_buf[0] = 0x81;                        // FIN + text frame
    frame_buf[1] = 0x80 | (msg_len & 0x7F);     // MASK bit + payload len
    
    //? 生成随机掩码
    uint8_t mask[4];
    for (int i = 0; i < 4; ++i) 
    {
        mask[i] = esp_random() & 0xFF;
    }
    memcpy(&frame_buf[2], mask, 4);
    
    //? 对消息内容进行掩码处理
    for (size_t i = 0; i < msg_len; ++i) 
    {
        frame_buf[6 + i] = msg[i] ^ mask[i % 4];
    }
    
    return msg_len + 6;
}

//? 解析WebSocket URI，提取主机名、端口和路径
static void parse_websocket_uri(const char *uri, char *host, int *port, char *path)
{
    const char *p = uri + strlen("ws://");
    const char *slash = strchr(p, '/');
    const char *colon = strchr(p, ':');
    
    *port = 80;     // 默认端口
    strcpy(path, "/");  // 默认路径
    
    if (colon && (!slash || colon < slash)) 
    {
        strncpy(host, p, colon - p);
        *port = atoi(colon + 1);
        if (slash) 
        {
            strncpy(path, slash, 127);
        }
    } 
    else 
    {
        if (slash) 
        {
            strncpy(host, p, slash - p);
            strncpy(path, slash, 127);
        } 
        else 
        {
            strncpy(host, p, 127);
        }
    }
}

//? WebSocket握手，返回socket文件描述符，失败返回-1
static int websocket_handshake(const char *uri)
{
    char host[128] = {0};
    char path[128] = {0};
    int port = 0;
    
    //? 解析URI
    parse_websocket_uri(uri, host, &port, path);
    ESP_LOGI(TAG, "Connecting to %s:%d%s", host, port, path);
    
    //? DNS解析
    struct addrinfo hints = {0}, *res;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    int retry = 0;
    while (getaddrinfo(host, NULL, &hints, &res) != 0 && retry < 3) 
    {
        ESP_LOGW(TAG, "DNS lookup failed, retry %d/3", retry + 1);
        vTaskDelay(pdMS_TO_TICKS(1000));
        retry++;
    }
    
    if (retry >= 3)
    {
        ESP_LOGE(TAG, "DNS lookup failed after 3 retries");
        return -1;
    }
    
    //? 创建socket
    struct sockaddr_in *addr = (struct sockaddr_in *)res->ai_addr;
    addr->sin_port = htons(port);
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) 
    {
        ESP_LOGE(TAG, "Socket create failed");
        freeaddrinfo(res);
        return -1;
    }
    
    //? 设置socket超时
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    
    //? 连接到服务器
    ESP_LOGI(TAG, "Attempting to connect to %s:%d", host, port);
    if (connect(sock, (struct sockaddr *)addr, sizeof(struct sockaddr_in)) != 0) 
    {
        ESP_LOGE(TAG, "Socket connect failed, errno: %d", errno);
        close(sock);
        freeaddrinfo(res);
        return -1;
    }
    freeaddrinfo(res);
    
    ESP_LOGI(TAG, "Socket connected successfully");
    
    //? 发送WebSocket握手请求
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
    
    if (send(sock, req, strlen(req), 0) <= 0)
    {
        ESP_LOGE(TAG, "Failed to send handshake request");
        close(sock);
        return -1;
    }
    
    //? 接收握手响应
    char resp[512];
    int len = recv(sock, resp, sizeof(resp)-1, 0);
    if (len <= 0) 
    {
        ESP_LOGE(TAG, "Handshake failed, no response");
        close(sock);
        return -1;
    }
    resp[len] = 0;
    ESP_LOGI(TAG, "WebSocket connected successfully");
    
    return sock;
}


//? WebSocket发送任务
static void wss_send_task(void *param)
{
    uint8_t frame_buf[256] = {0};
    
    while (1)
    {
        //? 检查socket是否有效
        if (g_websocket_sock < 0)
        {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        
        //? 发送消息到服务器
        const char *msg = "hello";
        size_t frame_len = build_websocket_frame(msg, frame_buf, sizeof(frame_buf));
        if (frame_len > 0) 
        {
            int ret = send(g_websocket_sock, frame_buf, frame_len, 0);
            if (ret > 0)
            {
                ESP_LOGI(TAG, "Sent: %s", msg);
            }
            else
            {
                ESP_LOGE(TAG, "Send failed");
                g_websocket_sock = -1;  // 标记连接断开
                break;
            }
        }
        
        //? 增加发送间隔，降低网络任务对音频任务的影响
        vTaskDelay(pdMS_TO_TICKS(2000));    // 从1秒改为2秒发送一次
    }
    
    ESP_LOGI(TAG, "wss_send_task ended");
    vTaskDelete(NULL);
}

//? WebSocket接收任务
static void wss_recv_task(void *param)
{
    char recv_buf[256] = {0};
    
    while (1)
    {
        //? 检查socket是否有效
        if (g_websocket_sock < 0)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }
        
        //? 接收服务器返回的消息
        uint8_t hdr[2];
        int r = recv(g_websocket_sock, hdr, 2, 0);
        if (r <= 0) 
        {
            ESP_LOGW(TAG, "Connection closed");
            g_websocket_sock = -1;  // 标记连接断开
            break;
        }
        
        //? 解析WebSocket帧头
        int payload_len = hdr[1] & 0x7F;
        if (payload_len > 0 && payload_len < sizeof(recv_buf)) 
        {
            memset(recv_buf, 0, sizeof(recv_buf));
            r = recv(g_websocket_sock, recv_buf, payload_len, 0);
            if (r > 0 && g_config && g_config->on_message) 
            {
                g_config->on_message(recv_buf, r);
            }
        }
    }
    
    ESP_LOGI(TAG, "wss_recv_task ended");
    vTaskDelete(NULL);
}

static void wss_client_task(void *param) 
{
    const wss_client_config_t *config = (const wss_client_config_t *)param;
    g_config = config;  // 保存配置供其他任务使用
    
    //? 参数有效性检查
    if (!config || !config->uri) 
    {
        ESP_LOGE(TAG, "Invalid WebSocket configuration");
        vTaskDelete(NULL);
        return;
    }

    //? 检查URI格式，仅支持 ws:// 协议
    if (strncmp(config->uri, "ws://", 5) != 0) 
    {
        ESP_LOGE(TAG, "Only ws:// protocol is supported");
        vTaskDelete(NULL);
        return;
    }

    //? 执行WebSocket握手连接，支持重试
    int sock = -1;
    int retry_count = 0;
    int max_retries = 5;
    
    while (sock < 0 && retry_count < max_retries)
    {
        if (retry_count > 0)
        {
            ESP_LOGW(TAG, "Retry connection %d/%d", retry_count, max_retries);
            vTaskDelay(pdMS_TO_TICKS(3000));  // 等待3秒后重试
        }
        
        sock = websocket_handshake(config->uri);
        retry_count++;
    }
    
    if (sock < 0)
    {
        ESP_LOGE(TAG, "WebSocket handshake failed after %d retries", max_retries);
        vTaskDelete(NULL);
        return;
    }
    
    g_websocket_sock = sock;    // 保存socket供其他任务使用
    
    //? 创建发送任务，优先级降低到3
    xTaskCreatePinnedToCore(wss_send_task, "wss_send", 4096, NULL, 3, NULL, 1);
    ESP_LOGI(TAG, "wss_send_task created");
    
    //? 创建接收任务，优先级降低到3
    xTaskCreatePinnedToCore(wss_recv_task, "wss_recv", 4096, NULL, 3, NULL, 1);
    ESP_LOGI(TAG, "wss_recv_task created");
    
    //? 主任务等待连接断开
    while (g_websocket_sock >= 0)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    close(sock);
    ESP_LOGI(TAG, "WebSocket connection closed");
    vTaskDelete(NULL);
}

void wss_client_start(const wss_client_config_t *config) 
{
    if (!config || !config->uri) 
    {
        ESP_LOGE(TAG, "Invalid config");
        return;
    }
    
    //? 降低WebSocket任务优先级到3，避免影响音频实时性
    xTaskCreatePinnedToCore(wss_client_task, "wss_client", 8192, (void *)config, 3, NULL, 1);
    ESP_LOGI(TAG, "wss_client_task created");
}

