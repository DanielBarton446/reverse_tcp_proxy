#pragma once

#include "event.h"

typedef enum PeerType {
    PEER_TYPE_CLIENT,
    PEER_TYPE_BACKEND,
} PeerType;

typedef struct TCPClient TCPClient;

struct TCPClient {
    EventBase event;

    PeerType peer_type;
    TCPClient* peer;

    uint8_t id;
    char* send_buf[1024];
    size_t send_len;
    size_t send_offset;
};


TCPClient* tcp_client_init(uint8_t id);
