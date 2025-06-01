/* #include "event.h" */
#include "server.h"
#include "worker.h"
#include "dbg.h"
#include <unistd.h>

int main(int argc, char* argv[]) {

    TCPServer* server = tcp_server_init(12345);

    tcp_server_start(server);
    log_info("Server started on port %d", server->port);

    run_worker_process(server);

    /* free(server); */
    /* free(es); */

    return 0;
}
