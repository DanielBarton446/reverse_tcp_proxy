/* #include "event.h" */
#include "event.h"
#include "server.h"
#include <stdio.h>
#include <unistd.h>



int main(int argc, char *argv[]) {

    EventSystem* es = event_system_init();
    TCPServer* server = tcp_server_init(42069);


    tcp_server_start(server);
    printf("Server started on port %d\n", server->port);
    // start event loop now
    es_add(es, (EventBase *) server, EPOLLIN);
    for (;;) {
        int nfds = es_wait(es, -1);

        for (int i = 0; i < nfds; i++) {
            EventBase* ev_data = (EventBase *) es->events[i].data.ptr;
            // uint32_t events = es->events[i].events;  // will use for send vs recv

            if (ev_data->type == SERVER_EVENT) {
                accept_client(server);
                printf("We found ourselves a client\n");
            } else {
            }
        }
    }

    /* free(server); */
    /* free(es); */

    return 0;
}
