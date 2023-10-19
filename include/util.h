#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdint.h>
#include <pthread.h>


#define PORT_SIZE 5   // max size for PORT number
#define PORT 1234
#define MAX_CLIENT_NUM 30   // max number of socket descriptors
#define TIMEOUT 600 // server timeout interval

#define h_addr h_addr_list[0] // for backward compatibility 

struct timeval tv = {TIMEOUT, 0};   // sleep for 10 minutes
typedef struct thread_params {
    char *buffer;
    int fd;
} thread_params;

void read_and_send_thread(int fd, char *buffer);
void listen_and_print_thread(int fd);
void *read_and_send(void *vargp);
void *listen_and_print(void *vargp);

int create_and_check_socket();
int bind_socket(int sockfd, struct sockaddr_in *address);
int listen_socket(int server_fd, int backlog);
int accept_new_socket(int server_fd, struct sockaddr_in *address, socklen_t *addrlen);

pthread_mutex_t mutex;

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


void read_and_send_thread(int fd, char *buffer)
{
    thread_params args;
    args.fd = fd;
    args.buffer = buffer;
    
    pthread_t id;
    pthread_create(&id, NULL, read_and_send, (void *)&args);
    //pthread_create(&id, NULL, read_and_send, (void *)(intptr_t)fd);
}

void listen_and_print_thread(int fd)
{
    pthread_t id;
    pthread_create(&id, NULL, listen_and_print, (void *)(intptr_t)fd);
    //pthread_create(&id, NULL, listen_and_print, (void *)(intptr_t)fd);
}

// int fd, char *buffer
void *read_and_send(void *vargp)
{
    struct thread_params *args = vargp;
    
    for (;;)
    {
        pthread_mutex_lock(&mutex);
        printf("Message: ");
        fgets(args->buffer, sizeof(args->buffer), stdin);
        args->buffer[strcspn(args->buffer, "\n")] = 0;

        // break loop for quitting
        if (strcmp(args->buffer, "quit") == 0)
            break;

        send(args->fd, args->buffer, strlen(args->buffer) + 1, 0);
        //sleep(2);
        pthread_mutex_unlock(&mutex);
    }
}

void *listen_and_print(void *vargp)
{
    int fd = (intptr_t)vargp;
    char buffer[1025];
    int valread;
    long int localA = 0;

    for (;;)
    {
        //pthread_mutex_init
        pthread_mutex_lock(&mutex);
        if ((valread = read(fd, buffer, 1024)) != 0)
        {
            printf("Client[%d]: %s\n", fd - 3, buffer);
        }
        //sleep(2);
        pthread_mutex_unlock(&mutex);
    }
    close(fd);
}

#endif