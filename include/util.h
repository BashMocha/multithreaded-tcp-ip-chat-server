#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdint.h>
#include <pthread.h>
#include <sqlite3.h>

#define PORT_SIZE 5           // max size for PORT number
#define MAX_CLIENT_NUM 10     // max number of socket descriptors
#define TIMEOUT 600           // server timeout interval
#define BUFF_SIZE 1024        // 1K size of buffer
#define h_addr h_addr_list[0] // for backward compatibility

struct timeval tv = {TIMEOUT, 0}; // sleep for 10 minutes

static bool terminated = false;
pthread_t tmp_thread;

pthread_t message_read_thread(int fd);
pthread_t message_send_thread(int fd);
void *message_read(void *vargp);
void *message_send(void *vargp);
int create_and_check_socket();
int bind_socket(int sockfd, struct sockaddr_in *address);
int listen_socket(int server_fd, int backlog);
int accept_new_socket(int server_fd, struct sockaddr_in *address, socklen_t *addrlen);

void create_db();
void insert_db();

// creates and checks creation process
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

// binds socket and checks bind process
int bind_socket(int server_fd, struct sockaddr_in *address)
{
    if (bind(server_fd, (struct sockaddr *)address, sizeof(*address)) < 0)
    {
        perror("Bind failed.\n");
        exit(EXIT_FAILURE);
    }
}

// prepare to accept connections from given socket FD
int listen_socket(int server_fd, int backlog)
{
    if (listen(server_fd, backlog) < 0)
    {
        perror("Listen failed.\n");
        exit(EXIT_FAILURE);
    }
}

// await a conection on socket FD and opens a new socket to communicate with it
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

// creates thread for read process and returns thread id
pthread_t message_read_thread(int fd)
{
    pthread_t id;
    pthread_create(&id, NULL, message_read, (void *)(intptr_t)fd);

    return id;
}

// creates thread for send process and returns thread id
pthread_t message_send_thread(int fd)
{
    pthread_t id;
    pthread_create(&id, NULL, message_send, (void *)(intptr_t)fd);

    return id;
}

// read 1K size of message from socket FD with a thread
void *message_read(void *vargp)
{
    bool disconnect = false;
    bool first_connection = true;
    intptr_t fd = (intptr_t)vargp;
    char buffer[BUFF_SIZE];
    int message_size = 0;

    tmp_thread = pthread_self();

    while (!disconnect)
    {
        if (first_connection)
        {
            if (recv(fd, buffer, BUFF_SIZE, 0) != 0)
            {
                printf("%s", buffer);
            }
            first_connection = false;
        }

        message_size = recv(fd, buffer, BUFF_SIZE, 0);
        if (message_size > 0)
        {
            printf("%s\n", buffer);
        }
        buffer[message_size] = 0; // set buffer empty
    }
}

// send 1K size of message to socket FD with a thread, and terminates both threads
void *message_send(void *vargp)
{
    bool disconnect = false;
    char buffer[BUFF_SIZE];
    int message_size = 0;
    intptr_t fd = (intptr_t)vargp;

    while (!disconnect)
    {
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = 0;
        if (strcmp(buffer, "quit") == 0)
        {
            terminated = true;
            break;
        }

        if (send(fd, buffer, strlen(buffer) + 1, 0) < 0)
        {
            printf("Server probably down.\n");
            disconnect = true;
        }
    }
    // if 'quit' is typed, terminate read-thread and then terminate send-thread
    pthread_cancel(tmp_thread);
    pthread_cancel(pthread_self());
}

// takes socket FD, converts to string and appends,
// when server gets a message, it redirects message to other clients with sender's socket FD at the end of the message
void send_other_client(char *buffer, int sender_fd, int receiver_fd)
{
    char num[12];
    char temp_arr[1025] = "Client[ ]: ";

    sprintf(num, "%d", sender_fd - 4);
    *(temp_arr + 7) = *(num + 0);
    strcat(temp_arr, buffer);
    send(receiver_fd, temp_arr, strlen(temp_arr) + 1, 0);
}

void create_db()
{
    sqlite3 *db;
    int rc = sqlite3_open("test.db", &db);
    char *sql1;
    int temp;
    char *zErrMsg = 0;

    if (rc)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    }
    else
    {
        fprintf(stderr, "Opened database successfully\n");
    }

    sql1 = "CREATE TABLE QUESTIONS("
           "ID INTEGER PRIMARY KEY      AUTOINCREMENT,"
           "QUESTION TEXT           NOT NULL,"
           "ANSWER TEXT             NOT NULL);";

    rc = sqlite3_exec(db, sql1, NULL, 0, &zErrMsg);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else
    {
        fprintf(stdout, "Table created successfully\n");
    }
    sqlite3_close(db);
}

void insert_db()
{
    sqlite3 *db;
    int rc = sqlite3_open("test.db", &db);
    char *sql1;
    char *zErrMsg = 0;

    if (rc)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    }
    else
    {
        fprintf(stderr, "Opened database successfully\n");
    }

    sql1 = "INSERT INTO QUESTIONS (ID, QUESTION, ANSWER) "
           "VALUES (1, 'Who is the first king of Portugal?', 'First Joao');";

    rc = sqlite3_exec(db, sql1, NULL, 0, &zErrMsg);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else
    {
        fprintf(stdout, "Records created successfully\n");
    }
    sqlite3_close(db);
}

#endif
