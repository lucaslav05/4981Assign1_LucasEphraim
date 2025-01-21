//
// Created by lucas-laviolette on 1/9/25.
//
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void setup_socket(int *sockfd);
void *handle_client(void *arg);
void build_http_response(const char  *file_name, const char *file_ext, char *response, ssize_t *response_size);

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

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handle_client, client_fd);
        pthread_detach(thread_id);


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
    return NULL;
}

void build_http_response(const char  *file_name, const char *file_ext, char *response, ssize_t *response_size) {

    const char *mime_type = "application/octet-stream";
    if (strcmp(file_ext, "html") == 0) {
        mime_type = "text/html";
    } else if (strcmp(file_ext, "txt") == 0) {
        mime_type = "text/plain";
    } else if (strcmp(file_ext, "png") == 0) {
        mime_type = "image/png";
    } else if(strcmp(file_ext, "jpg" ) || strcmp(file_ext, "jpeg") == 0) {
        mime_type = "image/jpeg";
    }

    //if not exist 404
    int file_fd = open(file_name, O_RDONLY);
    if(file_fd == -1) {
        snprintf(response, BUFFER_SIZE,
                 "HTTP/1.1 404 Not Found\r\n"
                 "Content-Type: text/plain\r\n"
                 "\r\n"
                 "404 Not Found");
        *response_size = strlen(response);
        return;
    }

    //get file size for Content-Length
    struct stat file_stat;
    fstat(file_fd, &file_stat);
    off_t file_size = file_stat.st_size;

    //Build http header
    char header[512];
    int header_length = snprintf(header, sizeof(header),
                                 "HTTP/1.1 200 OK\r\n"
                                 "Content-Type: %s\r\n"
                                 "Content-Length: %ld\r\n"
                                 "\r\n",
                                 mime_type, file_size);

    //copy header to response buffer
    *response_size = 0;
    memcpy(response, header, strlen(header));
    *response_size = strlen(header);

    //copy file to response buffer
    ssize_t bytes_read;
    while ((bytes_read = read(file_fd,
                             response + *response_size,
                             BUFFER_SIZE - *response_size)) > 0) {
        *response_size += bytes_read;
    }
    close(file_fd);
}
