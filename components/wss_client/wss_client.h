#ifndef _WSS_CLIENT_H
#define _WSS_CLIENT_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"

//? ==================== WebSocket默认配置 ====================
//? 可在调用 wss_client_start() 时传入自定义配置覆盖

//? WebSocket服务器URI（仅支持ws://，不支持wss://）
#ifndef WSS_URI
#define WSS_URI         "ws://124.222.6.60:8800"
#endif

//? WebSocket连接超时（秒）
#ifndef WSS_TIMEOUT_SEC
#define WSS_TIMEOUT_SEC 10
#endif

//? WebSocket重连间隔（毫秒）
#ifndef WSS_RETRY_DELAY_MS
#define WSS_RETRY_DELAY_MS  3000
#endif

//? WebSocket最大重连次数
#ifndef WSS_MAX_RETRY
#define WSS_MAX_RETRY   5
#endif

//? WebSocket消息发送间隔（毫秒）
#ifndef WSS_SEND_INTERVAL_MS
#define WSS_SEND_INTERVAL_MS  2000
#endif

//? WebSocket任务优先级
#ifndef TASK_WSS_PRIORITY
#define TASK_WSS_PRIORITY       3
#endif

//? WebSocket任务堆栈大小（字节）
#ifndef TASK_WSS_STACK_SIZE
#define TASK_WSS_STACK_SIZE     8192
#endif



#ifdef __cplusplus
extern "C" {
#endif

typedef void (*wss_on_message_cb)(const char *msg, size_t len);

typedef struct {
    const char *uri;
    wss_on_message_cb on_message;
} wss_client_config_t;

void wss_client_start(const wss_client_config_t *config);

#ifdef __cplusplus
}
#endif

#endif