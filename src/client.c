#include "client.h"
#include "event.h"
#include "dbg.h"
#include <stdlib.h>

TCPClient* tcp_client_init(uint8_t id) {
    TCPClient* client = (TCPClient*) malloc(sizeof(TCPClient));
    client->event.fd = -1;
    client->event.type = DOWNSTREAM_EVENT;
    client->id = id;

    log_info("we created a client with id %d\n", id);

    return client;
}
