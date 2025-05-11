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

static void accept_client(EventSystem* es, TCPServer* server);
static void cleanup_client(EventSystem* es, TCPClient* client);
static void recv_client(EventSystem* es, TCPClient* client);
static void send_tcp_client(EventSystem* es, TCPClient* client);
static void tx_echo(EventSystem* es, TCPClient* client);

static void create_associated_upstream(EventSystem* es, TCPClient* client);

static void tx_upstream_copy(EventSystem* es, TCPClient* client);

static void tx_downstream_copy(EventSystem* es, TCPClient* client);

void run_worker_process(TCPServer* server) {
    EventSystem* es = event_system_init();

    // start event loop now
    es_add(es, (EventBase*) server, EPOLLIN);
    int rx_before_len = 0;
    int rx_before_offset = 0;
    for (;;) {
        int nfds = es_wait(es, -1);

        for (int i = 0; i < nfds; i++) {
            EventBase* ev_data = (EventBase*) es->events[i].data.ptr;
            uint32_t events = es->events[i].events; // will use for send vs recv

            switch (ev_data->type) {
            case SERVER_EVENT:
                accept_client(es, (TCPServer*) ev_data);
                break;
            case DOWNSTREAM_EVENT:
                if (events & EPOLLIN) {
                    recv_client(es, (TCPClient*) ev_data);
                    tx_upstream_copy(es, (TCPClient*) ev_data); // prep send buff
                    tx_echo(es, (TCPClient*) ev_data);          // echo to client
                }
                if (events & EPOLLOUT) {
                    send_tcp_client(es, (TCPClient*) ev_data);
                    // Fully flushed, Strictly check rx from downstream
                    es_mod(es, ev_data, EPOLLIN);
                }
                break;
            case UPSTREAM_EVENT:
                if (events & EPOLLIN) {
                    recv_client(es, (TCPClient*) ev_data);
                    tx_downstream_copy(es, (TCPClient*) ev_data); // prep send buff
                }
                if (events & EPOLLOUT) {
                    send_tcp_client(es, (TCPClient*) ev_data);
                    // Fully flushed. Strictly check rx from upstream
                    es_mod(es, ev_data, EPOLLIN);
                }
                break;
            }
        }
    }
}

static void tx_echo(EventSystem* es, TCPClient* client) {
    // sanity check that client pointer exists
    if (client == NULL) {
        printf("Client not initialized\n");
        return;
    }

    // memcopy rx to tx
    client->tx_len = client->rx_len;
    client->tx_offset = client->rx_offset;
    memcpy(client->tx_buf, client->rx_buf, client->rx_len);

    if (client->tx_offset < client->tx_len) {
        es_mod(es, (EventBase*) client, EPOLLIN | EPOLLOUT);
    } else {
        es_mod(es, (EventBase*) client, EPOLLIN); // Clean state
    }
}

static void tx_downstream_copy(EventSystem* es, TCPClient* upstream) {
    // sanity check that downstream pointer exists
    if (upstream->peer == NULL) {
        printf("End client not initialized\n");
        return;
    }
    if (upstream->peer->peer_type != PEER_TYPE_DOWNSTREAM) {
        printf("Peer is not a client\n");
        return;
    }
    TCPClient* downstream = upstream->peer;

    // memcopy from upstream rx to downstream tx
    downstream->tx_len = upstream->rx_len;
    downstream->tx_offset = upstream->rx_offset;
    memcpy(downstream->tx_buf, upstream->rx_buf, upstream->rx_len);

    if (downstream->tx_offset < downstream->tx_len) {
        es_mod(es, (EventBase*) downstream, EPOLLIN | EPOLLOUT);
    } else {
        es_mod(es, (EventBase*) downstream, EPOLLIN); // Clean state
    }
}

static void tx_upstream_copy(EventSystem* es, TCPClient* downstream) {
    // Sanity check that the upstream pointer exists
    if (downstream->peer == NULL) {
        printf("Upstream client not initialized\n");
        return;
    }
    if (downstream->peer->peer_type != PEER_TYPE_UPSTREAM) {
        printf("Peer is not a upstream client\n");
        return;
    }
    TCPClient* upstream = downstream->peer;
    // memcopy from downstream rx to upstream tx
    upstream->tx_len = downstream->rx_len;
    upstream->tx_offset = downstream->rx_offset;
    memcpy(upstream->tx_buf, downstream->rx_buf, downstream->rx_len);

    if (upstream->tx_offset < upstream->tx_len) {
        es_mod(es, (EventBase*) upstream, EPOLLIN | EPOLLOUT);
    } else {
        es_mod(es, (EventBase*) upstream, EPOLLIN); // Clean state
    }
}

static void create_associated_upstream(EventSystem* es, TCPClient* downstream) {
    // hard code for now for upstream connection
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Failed to create socket");
        return;
    }

    set_non_blocking(sockfd);

    struct sockaddr_in upstream_addr;
    int upstream_addr_len = sizeof(upstream_addr);
    upstream_addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, "127.0.0.1", &upstream_addr.sin_addr) <= 0) {
        perror("Invalid address");
        return;
    }
    upstream_addr.sin_port = htons(6379); // pretend this port is correct

    if (connect(sockfd, (struct sockaddr*) &upstream_addr, upstream_addr_len) == -1) {
        if (errno != EINPROGRESS) {
            perror("Failed to connect to upstream");
            return;
        }
    }

    // make TCPClient for upstream
    TCPClient* upstream = tcp_client_init(++global_id);
    upstream->peer_type = PEER_TYPE_UPSTREAM;
    upstream->event.type = UPSTREAM_EVENT;
    upstream->event.fd = sockfd;

    // assocation of the peers
    upstream->peer = downstream;
    downstream->peer = upstream;

    // Now ADD UPSTREAM TO EVENT SYSTEM
    // -- not ready for sending from downstream -> upstream
    es_add(es, (EventBase*) upstream, EPOLLIN);

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
    client->peer_type = PEER_TYPE_DOWNSTREAM;

    client->event.fd = client_fd;

    create_associated_upstream(es, client);

    es_add(es, (EventBase*) client, EPOLLIN);

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
        ssize_t num_bytes = recv(client->event.fd, client->rx_buf, sizeof(client->rx_buf), 0);

        if (num_bytes > 0) {
            client->rx_len = num_bytes;
            client->rx_offset = 0;
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
    while (client->tx_offset < client->tx_len) {
        ssize_t n = send(client->event.fd, client->tx_buf + client->tx_offset,
                         client->tx_len - client->tx_offset, 0);

        if (n == -1) {
            perror("we failed to send for some reason\n");
            // TODO: more cleanup stuff
        }
        printf("Sent %ld bytes to client %d\n", n, client->id);

        client->tx_offset += n;
    }

    // we have flushed buffer best we can
    client->tx_len = 0;
    client->tx_offset = 0;

    return;
}
