#include "server.h"
#include "event.h"
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

TCPServer* tcp_server_init(uint16_t port) {
    TCPServer* server = (TCPServer*) malloc(sizeof(TCPServer));

    server->port = port;
    server->event.fd = 0;
    server->event.type = SERVER_EVENT;

    return server;
}


void tcp_server_start(TCPServer* server) {
    int sockfd = socket(AF_INET, SOCK_STREAM, NULL);
    if (sockfd == -1) {
        perror("Failed to create socket for tcp server");
        perror(errno);
    }
    
    // allow reuse of ip
    int opt = 1;
    int opt_res = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (opt_res == -1) {
        perror("Opt Bruh");
        perror(errno);
    }

    struct sockaddr_in sock_info;
    sock_info.sin_family = AF_INET;
    sock_info.sin_addr.s_addr = INADDR_ANY;
    sock_info.sin_port = htons(server->port);

    int bind_res = bind(sockfd, (struct sockaddr *) &sock_info, sizeof(sock_info));
    if (bind_res == -1) {
        perror("Bind Bruh");
        perror(errno);
    }

    int listen_res = listen(sockfd, SOMAXCONN);

    return;
}

void tcp_server_stop(TCPServer* server) {
    close(server->event.fd);
    return;
}
