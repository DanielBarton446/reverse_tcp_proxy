#pragma once

#include <sys/epoll.h>

#define MAX_EVENTS 1024


typedef enum EventType {
    SERVER_EVENT,
    CLIENT_EVENT,
    BACKEND_EVENT,
} EventType;


typedef struct EventBase {
    int fd;
    EventType type;
} EventBase;


typedef struct EventSystem {
    int epoll_fd;
    struct epoll_event events[MAX_EVENTS];
} EventSystem;


EventSystem* event_system_init();

int es_add(EventSystem* es, EventBase* data, uint32_t events);
int es_mod(EventSystem* es, EventBase* data, uint32_t events);
int es_del(EventSystem* es, int fd); // unregister fd from interest list
int es_wait(EventSystem* es, int timeout);
