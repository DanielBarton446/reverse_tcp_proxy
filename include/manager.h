#pragma once

#include "server.h"
#include <sys/types.h>

#define MAX_WORKERS 10

typedef struct Manager {
    TCPServer* server;
    uint8_t num_workers;
    pid_t manager_pid;
    pid_t worker_pids[MAX_WORKERS];
    // Eventually we will have references to a config reader

} Manager;


Manager* manager_init(uint16_t port);

void run_manager(Manager* manager);
void free_manager(Manager* manager);
