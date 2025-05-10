#include "event.h"
#include "server.h"
#include <unistd.h>



int main(int argc, char *argv[]) {

    EventSystem* es = event_system_init();
    TCPServer* server = tcp_server_init(42069);

    tcp_server_start(server);
    for (;;) {
        sleep(1);
    }

    /* free(server); */
    /* free(es); */

    return 0;
}
