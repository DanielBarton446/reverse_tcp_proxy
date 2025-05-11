#include "worker.h"
#include "client.h"
#include "event.h"
#include <sys/types.h>
#include <unistd.h>

#include <netinet/in.h>
#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>

uint8_t global_id = 0;



static void accept_client(EventSystem* es, TCPServer *server);
static void recv_client(EventSystem* es, TCPClient *client);
static void echo_client(EventSystem* es, TCPClient *client);



void run_worker_process(TCPServer *server) {
    EventSystem* es = event_system_init();

    // start event loop now
    es_add(es, (EventBase *) server, EPOLLIN);
    for (;;) {
        int nfds = es_wait(es, -1);

        for (int i = 0; i < nfds; i++) {
            EventBase* ev_data = (EventBase *) es->events[i].data.ptr;
            uint32_t events = es->events[i].events;  // will use for send vs recv
        
            switch (ev_data->type) {
                case SERVER_EVENT:
                    accept_client(es, (TCPServer *) ev_data);
                    printf("We found ourselves a client\n");
                    break;
                case CLIENT_EVENT:
                    if (events & EPOLLIN) {
                        // time to read!
                        printf("some client event\n");
                        recv_client(es, (TCPClient *) ev_data);
                    }
                    if (events & EPOLLOUT) {
                        echo_client(es, (TCPClient *) ev_data);
                    }
                    break;
            }
        }
    }

}


static void accept_client(EventSystem* es, TCPServer *server) {
    struct sockaddr_in client_addr;
    socklen_t client_addr_size = sizeof(client_addr);
    int client_fd = accept(server->event.fd, (struct sockaddr *) &client_addr, &client_addr_size);
    if (client_fd == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            printf("Failed client accept\n");
        }
    }
    set_non_blocking(client_fd);
    TCPClient* client = tcp_client_init(++global_id);
    client->event.fd = client_fd;
    
    es_add(es, (EventBase *) client, EPOLLIN);
    return;
}


static void recv_client(EventSystem* es, TCPClient* client) {
    while(1) {
        ssize_t num_bytes = recv(client->event.fd, client->send_buf, sizeof(client->send_buf), 0);

        if (num_bytes > 0) {
            client->send_len = num_bytes;
            client->send_offset = 0;

            es_mod(es, (EventBase *) client, EPOLLIN | EPOLLOUT);
            // mark ready for epoll out.
        } else if (num_bytes == 0) {
            printf("Client %d Disconnected", client->id);
            es_del(es, client->event.fd);
            break;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            } else {
                perror("we really failed hard on recv_client");
                // prolly some unresolved cleanup
                break;
            }
        }
    }
}


static void echo_client(EventSystem* es, TCPClient* client) {
    while (client->send_offset < client->send_len) {
        ssize_t n = send(client->event.fd,
                         client->send_buf + client->send_offset,
                         client->send_len - client->send_offset,
                         0);
        // we might have to do some validations here on 
        // number of bytes sent vs buffer size.
        // I think we need to fully flush the buffer 
        // so we dont end up with recv overwriting before sending.
        
        if (n == -1) {
            perror("we failed to send for some reason");
            // TODO: more cleanup stuff
        }

        client->send_offset += n;
    }

    // we have flushed buffer best we can
    client->send_len = 0;
    client->send_offset = 0;

    es_mod(es, (EventBase *) client, EPOLLIN);
    
    return;
}
