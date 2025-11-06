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