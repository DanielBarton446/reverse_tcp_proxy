#pragma once

#include "event.h"

typedef struct TCPServer {
    // we want the TCPServer to 
    // be an event so we can do things with it
    EventBase event;
    uint16_t port;
} TCPServer;


TCPServer* tcp_server_init(uint16_t port);
void tcp_server_start(TCPServer* server);
void tcp_server_stop(TCPServer* server);

