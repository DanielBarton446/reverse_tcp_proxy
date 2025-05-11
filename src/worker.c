#include "worker.h"
#include "client.h"
#include "event.h"
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>

uint8_t global_id = 0;

static void create_client_backend(EventSystem* es, TCPClient* client);
static void forward_to_backend(EventSystem* es, TCPClient* client);

static void accept_client(EventSystem* es, TCPServer* server);
static void cleanup_client(EventSystem* es, TCPClient* client);

static void recv_client(EventSystem* es, TCPClient* client);
static void send_tcp_client(EventSystem* es, TCPClient* client);

void run_worker_process(TCPServer* server) {
    EventSystem* es = event_system_init();

    // start event loop now
    es_add(es, (EventBase*) server, EPOLLIN);
    int send_before_len = 0;
    int send_before_offset = 0;
    for (;;) {
        int nfds = es_wait(es, -1);

        for (int i = 0; i < nfds; i++) {
            EventBase* ev_data = (EventBase*) es->events[i].data.ptr;
            uint32_t events = es->events[i].events; // will use for send vs recv

            switch (ev_data->type) {
            case SERVER_EVENT:
                accept_client(es, (TCPServer*) ev_data);
                break;
            case CLIENT_EVENT:
                if (events & EPOLLIN) {
                    recv_client(es, (TCPClient*) ev_data);
                }
                if (events & EPOLLOUT) {
                    send_before_len = ((TCPClient*) ev_data)->send_len;
                    send_before_offset = ((TCPClient*) ev_data)->send_offset;
                    send_tcp_client(es, (TCPClient*) ev_data); // echo
                    // reset so we can also forward to backend
                    ((TCPClient*) ev_data)->send_len = send_before_len;
                    ((TCPClient*) ev_data)->send_offset = send_before_offset;
                    forward_to_backend(es, (TCPClient*) ev_data);
                }
                break;
            case BACKEND_EVENT:
                if (events & EPOLLIN) {
                    printf("some backend event\n");
                    // at this point, forward to backend tcp client
                }
                if (events & EPOLLOUT) {
                    // we have data to send to backend
                    // likely created from handling CLIENT_EVENT
                }
            }
        }
    }
}

static void forward_to_backend(EventSystem* es, TCPClient* end_client) {
    // Sanity check that the backend pointer exists
    if (end_client->peer == NULL) {
        printf("Backend client not initialized\n");
        return;
    }
    // need to mem copy from client send buffer to backend send buffer
    end_client->peer->send_len = end_client->send_len;
    end_client->peer->send_offset = end_client->send_offset;
    memcpy(end_client->peer->send_buf, end_client->send_buf, end_client->send_len);

    printf("Forwarding data to backend\n");
    send_tcp_client(es, end_client->peer); 

    // we have sent everything, so reset flags since no more
    // EPOLLOUT at this moment.
    es_mod(es, (EventBase*) end_client, EPOLLIN);
}

static void create_client_backend(EventSystem* es, TCPClient* end_client) {
    // hard code for now for backend connection
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Failed to create socket");
        return;
    }

    set_non_blocking(sockfd);

    struct sockaddr_in backend_addr;
    int backend_addr_len = sizeof(backend_addr);
    backend_addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, "127.0.0.1", &backend_addr.sin_addr) <= 0) {
        perror("Invalid address");
        return;
    }
    backend_addr.sin_port = htons(8000); // pretend this port is correct
    //
    if (connect(sockfd, (struct sockaddr*) &backend_addr, backend_addr_len) == -1) {
        if (errno != EINPROGRESS) {
            perror("Failed to connect to backend");
            return;
        }
    }
    // make TCPClient for backend
    TCPClient* backend_client = tcp_client_init(++global_id);
    backend_client->peer_type = PEER_TYPE_BACKEND;
    backend_client->event.fd = sockfd;

    // assocation of the peers
    backend_client->peer = end_client;
    end_client->peer = backend_client;

    // Now ADD BACKEND TO EVENT SYSTEM
    // -- not ready for sending from end_client -> backend.
    es_add(es, (EventBase*) backend_client, EPOLLIN);

    // I don't think end client needs to be ready for EPOLLOUT events now.

    return;
}

static void accept_client(EventSystem* es, TCPServer* server) {
    struct sockaddr_in client_addr;
    socklen_t client_addr_size = sizeof(client_addr);
    int client_fd = accept(server->event.fd, (struct sockaddr*) &client_addr, &client_addr_size);
    if (client_fd == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            printf("Failed client accept\n");
        }
    }
    set_non_blocking(client_fd);
    TCPClient* client = tcp_client_init(++global_id);
    client->peer_type = PEER_TYPE_CLIENT;

    client->event.fd = client_fd;

    es_add(es, (EventBase*) client, EPOLLIN);

    create_client_backend(es, client);

    return;
}

static void cleanup_client(EventSystem* es, TCPClient* client) {
    if (client->peer != NULL) {
        // remove from event system
        printf("Cleaned up Peer %d\n", client->peer->id);
        es_del(es, client->peer->event.fd);
        close(client->peer->event.fd);
        free(client->peer);
    }
    printf("Cleaning up Peer %d\n", client->id);
    es_del(es, client->event.fd);
    close(client->event.fd);
    free(client);
}

static void recv_client(EventSystem* es, TCPClient* client) {
    while (1) {
        ssize_t num_bytes = recv(client->event.fd, client->send_buf, sizeof(client->send_buf), 0);

        if (num_bytes > 0) {
            client->send_len = num_bytes;
            client->send_offset = 0;

            es_mod(es, (EventBase*) client, EPOLLIN | EPOLLOUT);
            // mark ready for epoll out.
        } else if (num_bytes == 0) {
            // EOF from client
            printf("Client %d Disconnected\n", client->id);
            cleanup_client(es, client);
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

static void send_tcp_client(EventSystem* es, TCPClient* client) {
    while (client->send_offset < client->send_len) {
        ssize_t n = send(client->event.fd, client->send_buf + client->send_offset,
                         client->send_len - client->send_offset, 0);
        // we might have to do some validations here on
        // number of bytes sent vs buffer size.
        // I think we need to fully flush the buffer
        // so we dont end up with recv overwriting before sending.

        if (n == -1) {
            perror("we failed to send for some reason\n");
            // TODO: more cleanup stuff
        }
        printf("Sent %ld bytes to client %d\n", n, client->id);

        client->send_offset += n;
    }

    // we have flushed buffer best we can
    client->send_len = 0;
    client->send_offset = 0;


    return;
}
