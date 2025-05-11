#pragma once

#include "event.h"

typedef enum PeerType {
    PEER_TYPE_DOWNSTREAM,
    PEER_TYPE_UPSTREAM,
} PeerType;

typedef struct TCPClient TCPClient;

struct TCPClient {
    EventBase event;

    PeerType peer_type;
    TCPClient* peer;

    uint8_t id;
    char* tx_buf[1024];
    size_t tx_len;
    size_t tx_offset;

    char* rx_buf[1024];
    size_t rx_len;
    size_t rx_offset;
};


TCPClient* tcp_client_init(uint8_t id);
