//
// Created by lucas-laviolette on 1/9/25.
//
#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define METHOD_SIZE 8
#define PATH_SIZE 256
#define PROTOCOL_SIZE 16
#define HEADER_SIZE 512

void  setup_socket(int *sockfd);
void *handle_client(void *arg);
void  build_http_response(const char *file_name, const char *file_ext, char *response, ssize_t *response_size);

int main(void)
{
    int server_fd;
    setup_socket(&server_fd);

    while(1)
    {
        pthread_t          thread_id;
        struct sockaddr_in client_addr;
        socklen_t          client_addr_len = sizeof(client_addr);
        int               *client_fd       = malloc(sizeof(int));

        *client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if(*client_fd < 0)
        {
            perror("accept failed");
            continue;
        }

        pthread_create(&thread_id, NULL, handle_client, client_fd);
        pthread_detach(thread_id);
    }
}

void setup_socket(int *sockfd)
{
    struct sockaddr_in server_addr;

    *sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(*sockfd < 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(PORT);

    if(bind(*sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if(listen(*sockfd, SOMAXCONN) < 0)
    {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
}

void *handle_client(void *arg)
{
    ssize_t     response_size;
    char        response[BUFFER_SIZE];
    const char *file_ext;
    const char *file_name;
    char        method[METHOD_SIZE];
    char        path[PATH_SIZE];
    char        protocol[PROTOCOL_SIZE];
    ssize_t     bytes_received;
    char        buffer[BUFFER_SIZE] = {0};
    int         client_fd           = *(int *)arg;
    free(arg);

    bytes_received = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
    if(bytes_received <= 0)
    {
        perror("recv failed");
        close(client_fd);
        return NULL;
    }

    if(sscanf(buffer, "%7s %255s %15s", method, path, protocol) != 3)
    {
        const char *bad_request_response = "HTTP/1.1 400 Bad Request\r\n"
                                           "Content-Type: text/plain\r\n"
                                           "\r\n"
                                           "400 Bad Request";
        send(client_fd, bad_request_response, strlen(bad_request_response), 0);
        close(client_fd);
        return NULL;
    }

    if(strcmp(method, "GET") != 0 && strcmp(method, "HEAD") != 0)
    {
        const char *method_not_allowed_response = "HTTP/1.1 405 Method Not Allowed\r\n"
                                                  "Content-Type: text/plain\r\n"
                                                  "\r\n"
                                                  "405 Method Not Allowed";
        send(client_fd, method_not_allowed_response, strlen(method_not_allowed_response), 0);
        close(client_fd);
        return NULL;
    }

    file_name = (path[0] == '/') ? path + 1 : path;

    file_ext = strrchr(file_name, '.');
    if(file_ext)
    {
        file_ext++;
    }
    else
    {
        file_ext = "";
    }

    build_http_response(file_name, file_ext, response, &response_size);

    if(strcmp(method, "HEAD") == 0)
    {
        const char *header_end = strstr(response, "\r\n\r\n");
        if(header_end)
        {
            ssize_t header_size = header_end - response + 4;
            send(client_fd, response, (size_t)header_size, 0);
        }
    }
    else
    {
        send(client_fd, response, (size_t)response_size, 0);
    }

    close(client_fd);
    return NULL;
}

void build_http_response(const char *file_name, const char *file_ext, char *response, ssize_t *response_size)
{
    ssize_t     bytes_read;
    char        header[HEADER_SIZE];
    off_t       file_size;
    struct stat file_stat;
    int         file_fd;
    const char *mime_type = "application/octet-stream";
    if(strcmp(file_ext, "html") == 0)
    {
        mime_type = "text/html";
    }
    else if(strcmp(file_ext, "txt") == 0)
    {
        mime_type = "text/plain";
    }
    else if(strcmp(file_ext, "png") == 0)
    {
        mime_type = "image/png";
    }
    else if(strcmp(file_ext, "jpg") == 0 || strcmp(file_ext, "jpeg") == 0)
    {
        mime_type = "image/jpeg";
    }

    // if not exist 404
    file_fd = open(file_name, O_RDONLY | O_CLOEXEC);
    if(file_fd == -1)
    {
        snprintf(response,
                 BUFFER_SIZE,
                 "HTTP/1.1 404 Not Found\r\n"
                 "Content-Type: text/plain\r\n"
                 "\r\n"
                 "404 Not Found");
        *response_size = (ssize_t)strlen(response);
        return;
    }

    // get file size for Content-Length
    fstat(file_fd, &file_stat);
    file_size = file_stat.st_size;

    // Build http header
    snprintf(header,
             sizeof(header),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %ld\r\n"
             "\r\n",
             mime_type,
             file_size);

    // copy header to response buffer
    *response_size = 0;
    memcpy(response, header, strlen(header));
    *response_size = (ssize_t)strlen(header);

    // copy file to response buffer
    while((bytes_read = read(file_fd, response + *response_size, (size_t)(BUFFER_SIZE - *response_size))) > 0)
    {
        *response_size += bytes_read;
    }
    close(file_fd);
}
