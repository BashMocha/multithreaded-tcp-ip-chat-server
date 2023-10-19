#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>

#define PORT_SIZE 5 // max size for PORT number
#define MAX_CLIENT_NUM 10     // max number of socket descriptors
#define TIMEOUT 600           // server timeout interval
#define h_addr h_addr_list[0] // for backward compatibility

struct timeval tv = {TIMEOUT, 0}; // sleep for 10 minutes

int create_and_check_socket();
int bind_socket(int sockfd, struct sockaddr_in *address);
int listen_socket(int server_fd, int backlog);
int accept_new_socket(int server_fd, struct sockaddr_in *address, socklen_t *addrlen);
void send_other_client(char *buffer, int sender_fd, int receiver_fd);


int create_and_check_socket()
{
    int server_fd;
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation failed.\n");
        exit(EXIT_FAILURE);
    }
    return server_fd;
}

int bind_socket(int server_fd, struct sockaddr_in *address)
{
    if (bind(server_fd, (struct sockaddr *)address, sizeof(*address)) < 0)
    {
        perror("Bind failed.\n");
        exit(EXIT_FAILURE);
    }
}

int listen_socket(int server_fd, int backlog)
{
    if (listen(server_fd, backlog) < 0)
    {
        perror("Listen failed.\n");
        exit(EXIT_FAILURE);
    }
}

int accept_new_socket(int server_fd, struct sockaddr_in *address, socklen_t *addrlen)
{
    int new_socket;

    if ((new_socket = accept(server_fd, (struct sockaddr *)address, addrlen)) < 0)
    {
        perror("Accept failed.\n");
        exit(EXIT_FAILURE);
    }
    return new_socket;
}

void send_other_client(char *buffer, int sender_fd, int receiver_fd)
{
    char num[12];
    char temp_arr[1025] = "Client[ ]: ";

    sprintf(num, "%d", sender_fd - 4);
    *(temp_arr + 7) = *(num + 0);
    strcat(temp_arr, buffer);
    send(receiver_fd, temp_arr, strlen(temp_arr) + 1, 0);
}

#endif