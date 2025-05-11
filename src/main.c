/* #include "event.h" */
#include "server.h"
#include "worker.h"
#include <stdio.h>
#include <unistd.h>



int main(int argc, char *argv[]) {

    TCPServer* server = tcp_server_init(12345);

    tcp_server_start(server);
    printf("Server started on port %d\n", server->port);

    run_worker_process(server);

    /* free(server); */
    /* free(es); */

    return 0;
}
