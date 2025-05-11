#include "server.h"
#include "event.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

TCPServer* tcp_server_init(uint16_t port) {
    TCPServer* server = (TCPServer*) malloc(sizeof(TCPServer));

    server->port = port;
    server->event.fd = -1;
    server->event.type = SERVER_EVENT;

    return server;
}

void tcp_server_start(TCPServer* server) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Failed to create socket for tcp server\n");
    }
    server->event.fd = sockfd;

    set_non_blocking(sockfd);

    // allow reuse of ip
    int opt = 1;
    int opt_res = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (opt_res == -1) {
        perror("Opt issues\n");
    }

    struct sockaddr_in sock_info;
    sock_info.sin_family = AF_INET;
    sock_info.sin_addr.s_addr = INADDR_ANY;
    sock_info.sin_port = htons(server->port);

    int bind_res = bind(sockfd, (struct sockaddr*) &sock_info, sizeof(sock_info));
    if (bind_res == -1) {
        perror("Bind issues\n");
    }

    int listen_res = listen(sockfd, SOMAXCONN);
    if (listen_res == -1) {
        perror("Listen issues\n");
    }
    return;
}

void tcp_server_stop(TCPServer* server) {
    close(server->event.fd);
    return;
}
