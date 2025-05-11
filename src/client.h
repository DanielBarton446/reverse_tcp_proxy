#pragma once

#include "event.h"

typedef struct TCPClient {
    EventBase event;
    uint8_t id;
    char* send_buf[1024];
    size_t send_len;
    size_t send_offset;
} TCPClient;


TCPClient* tcp_client_init(uint8_t id);
