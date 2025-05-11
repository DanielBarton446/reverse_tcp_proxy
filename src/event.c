#include "event.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>

int es_add(EventSystem* es, EventBase* data, uint32_t events) {
    struct epoll_event event;
    // allows us to per-event typecast and do things with events
    event.data.ptr = data;
    event.events = events;

    return epoll_ctl(es->epoll_fd, EPOLL_CTL_ADD, data->fd, &event);
}

int es_del(EventSystem* es, int fd) {
    return epoll_ctl(es->epoll_fd, EPOLL_CTL_DEL, fd, NULL);
}

int es_wait(EventSystem* es, int timeout) {
    return epoll_wait(es->epoll_fd, es->events, MAX_EVENTS, timeout);
}

int es_mod(EventSystem* es, EventBase* data, uint32_t events) {
    struct epoll_event event;
    event.data.ptr = data;
    event.events = events;

    return epoll_ctl(es->epoll_fd, EPOLL_CTL_MOD, data->fd, &event);
}

EventSystem* event_system_init() {
    EventSystem* event_system = (EventSystem*) malloc(sizeof(EventSystem));

    event_system->epoll_fd = epoll_create1(0);
    memset(event_system->events, 0, sizeof(event_system->events));

    return event_system;
}
