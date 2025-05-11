#include "client.h"
#include "event.h"
#include <stdio.h>
#include <stdlib.h>



TCPClient* tcp_client_init(uint8_t id) {
    TCPClient* client = (TCPClient *) malloc(sizeof(TCPClient));
    client->event.fd = -1;
    client->event.type = CLIENT_EVENT;
    client->id = id;

    printf("we created a client with id %d\n", id);

    return client;
}
