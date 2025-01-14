//
// Created by lucas-laviolette on 1/9/25.
//
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>

#define PORT 8080

void setup_socket(int *sockfd);
void *handle_client(void *arg);

int main() {

    int server_fd;
    setup_socket(&server_fd);

    while (1) {

        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int *client_fd = malloc(sizeof(int));


        if ((*client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len)) < 0) {
            perror("accept failed");
            continue;
        }


    }



    return 0;
}

void setup_socket(int *sockfd) {
    struct sockaddr_in server_addr;

    if ((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(*sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(*sockfd, SOMAXCONN) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
}

void *handle_client(void *arg) {
    //MEOWw
}
