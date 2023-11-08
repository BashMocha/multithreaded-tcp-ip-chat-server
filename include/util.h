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
#define SERVER_DB "SERVER.db" // server. db file

struct timeval tv = {TIMEOUT, 0}; // sleep for 10 minutes

static bool terminated = false;
pthread_t tmp_thread;
char commands[3][15] = {"ASK ", "ANSWER -", "LISTQUESTIONS"};

pthread_t message_read_thread(int fd);
pthread_t message_send_thread(int fd);
void *message_read(void *vargp);
void *message_send(void *vargp);
int create_and_check_socket();
int bind_socket(int sockfd, struct sockaddr_in *address);
int listen_socket(int server_fd, int backlog);
int accept_new_socket(int server_fd, struct sockaddr_in *address, socklen_t *addrlen);

void create_db();
void ask_db(char *buffer);
void answer_db(char *buffer, char id);
void listquestions_db(char *buffer, int questions);

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
        else if (strcmp(buffer, "help") == 0)
        {
            printf("use 'ASK ...' for questions.\nuse 'ANSWER -{id} ...' for answer a question.\nuse 'LISTQUESTIONS' for listing all questions.\n");
            continue;
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
    int rc = sqlite3_open("SERVER.db", &db);
    char *sql1;
    int temp;
    char *ErrMsg = 0;

    if (rc)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    }
    else
    {
        fprintf(stderr, "Opened database successfully\n");
    }

    sql1 = "CREATE TABLE QUESTIONS("
           "ID INTEGER PRIMARY KEY,"
           "QUESTION TEXT               NOT NULL,"
           "ANSWER TEXT                 DEFAULT '');";

    rc = sqlite3_exec(db, sql1, NULL, 0, &ErrMsg);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", ErrMsg);
        sqlite3_free(ErrMsg);
    }
    else
    {
        fprintf(stdout, "Table created successfully\n");
    }
    sqlite3_close(db);
}

void ask_db(char *buffer)
{
    sqlite3 *db;
    int rc = sqlite3_open(SERVER_DB, &db);
    char *sql1;
    char *ErrMsg = 0;

    if (rc)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    }

    sql1 = sqlite3_mprintf("INSERT INTO QUESTIONS (QUESTION, ANSWER) VALUES('%s', '%s')", buffer, "NOTANSWERED");
    rc = sqlite3_exec(db, sql1, NULL, 0, &ErrMsg);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", ErrMsg);
        sqlite3_free(ErrMsg);
    }
    sqlite3_close(db);
}

void answer_db(char *buffer, char c)
{
    sqlite3 *db;
    int rc = sqlite3_open(SERVER_DB, &db);
    char *sql1;
    char *ErrMsg = 0;
    int id = c - '0';

    if (rc)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    }
    sql1 = sqlite3_mprintf("UPDATE QUESTIONS SET ANSWER='%s' WHERE id= '%d'", buffer, id);

    rc = sqlite3_exec(db, sql1, NULL, 0, &ErrMsg);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", ErrMsg);
        sqlite3_free(ErrMsg);
    }

    // free(answer);
    sqlite3_close(db);
}

void listquestions_db(char *buffer, int questions)
{
    sqlite3 *db;
    sqlite3_stmt *res;
    sqlite3_stmt *stmt;

    int rc = sqlite3_open(SERVER_DB, &db);
    int count = 0;
    char *sql1;
    const char *tail;
    char *ErrMsg = 0;

    if (rc)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    }
    if (sqlite3_prepare_v2(db, "SELECT QUESTION,ANSWER FROM QUESTIONS", 128, &res, &tail) != SQLITE_OK)
    {
        printf("Can't retrieve data: %s\n", sqlite3_errmsg(db));
    }

    size_t len = strlen(buffer);
    size_t command_len = strlen("LISTQUESTIONS");
    memmove(buffer, buffer + command_len, len - command_len + 1);
    while (sqlite3_step(res) == SQLITE_ROW)
    {
        count += 1;
        char *temp_q = (char *)malloc(100 * sizeof(char));
        sprintf(temp_q, "\n(%d) %s\n    %s\n", count, sqlite3_column_text(res, 0), sqlite3_column_text(res, 1));
        strcat(buffer, temp_q);
        free(temp_q);
    }
    strcat(buffer, "\nENDQUESTIONS\n");
    sqlite3_finalize(res);
    sqlite3_close(db);
}

#endif