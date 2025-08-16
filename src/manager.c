#include "manager.h"
#include "colors.h"
#include "server.h"
#include "worker.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <unistd.h>

static void spawn_workers(Manager* manager);
static void print_server_banner(Manager* manager);

Manager* manager_init(uint16_t port) {
    Manager* manager = (Manager*) malloc(sizeof(Manager));

    TCPServer* server = tcp_server_init(port);
    tcp_server_start(server);

    manager->server = server;
    manager->manager_pid = getpid();
    manager->num_workers = 3;

    // This works strictly for pgrep
    prctl(PR_SET_NAME, "ProxyManager", 0, 0, 0);

    return manager;
}

void spawn_workers(Manager* manager) {
    for (size_t i = 0; i < manager->num_workers; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("Failed to fork worker process");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            prctl(PR_SET_NAME, "ProxyWorker", 0, 0, 0);
            run_worker_process(manager->server);

            // maybe worker needs to cleanup master on shutdown?

            exit(EXIT_SUCCESS);
        }
    }
}

void run_manager(Manager* manager) {
    print_server_banner(manager);

    spawn_workers(manager);

    for (;;) {
        // Keep Manager running until we are ready to shutdown
        sleep(1);
    }

    // SIGTERM all childrean
    /* kill(0, SIGTERM); */
}

void free_manager(Manager* manager) {
    // what about all the event systems?
    tcp_server_stop(manager->server);
    free(manager->server);
    free(manager);
}

void print_server_banner(Manager* manager) {
    // its fine to printf here

    printf(COLOR_CYAN "============================\n" COLOR_YELLOW
                      "PRODUCT NAME PENDING\n" COLOR_CYAN
                      "============================\n" COLOR_RESET);
}
