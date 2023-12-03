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
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <math.h>

#define PORT_SIZE 5       // max size for PORT number
#define MAX_CLIENT_NUM 30 // max number of socket descriptors
#define TIMEOUT 600       // server timeout interval
#define BUFF_SIZE 1024    // 1KB size of buffer
#define NAME_SIZE 15
#define h_addr h_addr_list[0] // for backward compatibility
#define SERVER_DB "SERVER.db" // server. db file

struct timeval tv = {TIMEOUT, 0}; // sleep for 10 minutes
static bool terminated = false;
pthread_t tmp_thread;
char commands[7][15] = {"ASK ", "ANSWER -", "LISTQUESTIONS", "PUTFILE ", "GETFILE ", "LISTFILES", "LISTUSERS"};

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
void listquestions_db(char *buffer);
int send_file(int socket_fd, char *filename);
int receive_file(int socket_fd, char *filename);
int get_file_size(char *filename);
void putfile_db(char *buffer);
void listfiles_db(char *buffer);
bool file_exists(char *filename);
void registered_db(char *client_name, int port_num, int action);
char *get_client_name(in_port_t port_number);
void listusers_db(char *buffer);

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

        if (strncmp(buffer, commands[4], strlen(commands[4])) == 0)
        {
            char path[BUFF_SIZE];

            strcpy(path, "./local/");
            const char seperator = ' ';
            char *filename = strchr(buffer, seperator);
            *filename = '\0';
            strcat(path, filename + 1);

            if ((receive_file(fd, path)) == -1)
            {
                printf("File could not received.\n");
            }
            continue;
        }
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
            printf("use 'ASK ...' to ask a question.\n"
                   "use 'ANSWER -{id} ...' to answer a question.\n"
                   "use 'LISTQUESTIONS' to list all questions.\n"
                   "use 'PUTFILE {file_name}' to upload a file to server.\n"
                   "use 'GETFILE -{id}' to download a file from server.\n"
                   "use 'LISTFILES' to list all files hold by server.\n"
                   "use 'LISTUSERS' to list all active users on server.\n");
            continue;
        }
        // PUTFILE
        else if (strncmp(buffer, commands[3], strlen(commands[3])) == 0)
        {
            char temp[BUFF_SIZE], path[BUFF_SIZE];
            strcpy(temp, buffer);
            strcpy(path, "./local/");

            const char seperator = ' ';
            char *const filename = strchr(buffer, seperator);
            *filename = '\0';
            strcat(path, filename + 1);

            if (!file_exists(path))
            {
                printf("File not found.\n");
                continue;
            }

            send(fd, temp, strlen(temp) + 1, 0);

            if ((send_file(fd, path)) == -1)
            {
                printf("File could not send.\n");
            }
            continue;
        }
        // GETFILE
        else if (strncmp(buffer, commands[4], strlen(commands[4])) == 0)
        {
            char path[BUFF_SIZE];
            strcpy(path, "./local/");

            send(fd, buffer, strlen(buffer) + 1, 0);
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

void create_db()
{
    sqlite3 *db;
    int rc = sqlite3_open("SERVER.db", &db);
    char *sql1;
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
        fprintf(stdout, "Question table created successfully\n");
    }

    sql1 = "CREATE TABLE FILES("
           "ID INTEGER PRIMARY KEY,"
           "FILENAME TEXT               NOT NULL);";

    rc = sqlite3_exec(db, sql1, NULL, 0, &ErrMsg);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", ErrMsg);
        sqlite3_free(ErrMsg);
    }
    else
    {
        fprintf(stdout, "Files table created successfully\n");
    }

    sql1 = "CREATE TABLE REGISTERED("
           "ID INTEGER PRIMARY KEY,"
           "PORT INTEGER    NOT NULL,"
           "NAME TEXT       NOT NULL);";

    rc = sqlite3_exec(db, sql1, NULL, 0, &ErrMsg);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", ErrMsg);
        sqlite3_free(ErrMsg);
    }
    else
    {
        fprintf(stdout, "Registered table created successfully\n");
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

    sqlite3_close(db);
}

void listquestions_db(char *buffer)
{
    sqlite3 *db;
    sqlite3_stmt *res;
    int rc = sqlite3_open(SERVER_DB, &db);
    int count = 0;
    const char *tail;

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

int send_file(int socket_fd, char *filename)
{
    int count, total, file_size, bytes_left, sent;
    uint32_t tmp;
    char temp_filename[255];
    char send_buffer[BUFF_SIZE];
    FILE *f;

    strcpy(temp_filename, filename);

    file_size = get_file_size(filename);
    tmp = htonl(file_size);
    send(socket_fd, &tmp, sizeof(uint32_t), 0);

    f = fopen(filename, "rb");

    total = 0;
    bytes_left = file_size;
    while (total < file_size)
    {
        sent = fread(send_buffer, 1, fmin(BUFF_SIZE, bytes_left), f);
        count = send(socket_fd, send_buffer, fmin(BUFF_SIZE, bytes_left), 0);
        // printf("%d\n", sent);

        if (count == -1)
            return -1;
        total += count;
        bytes_left -= count;
        memset(send_buffer, 0, BUFF_SIZE);
    }
    fclose(f);

    recv(socket_fd, send_buffer, BUFF_SIZE, 0);
    if (strcmp(send_buffer, "FIN") == 0)
        printf("[+]Uploaded %s %d\n", temp_filename, file_size);
    return 1;
}

int receive_file(int socket_fd, char *filename)
{
    int count, total, received;
    uint32_t tmp, file_size;
    char temp_filename[255];
    char receive_buffer[BUFF_SIZE];
    FILE *f;

    recv(socket_fd, &tmp, sizeof(uint32_t), 0);
    file_size = ntohl(tmp);

    strcpy(temp_filename, filename);

    f = fopen(filename, "wb");

    total = 0;
    while (total < file_size)
    {
        received = recv(socket_fd, receive_buffer, BUFF_SIZE, 0);
        // printf("%d\n", received);

        total += received;
        fwrite(receive_buffer, 1, received, f);
        memset(receive_buffer, 0, BUFF_SIZE);
    }
    fclose(f);

    send(socket_fd, "FIN", strlen("FIN") + 1, 0);
    printf("[+]FILE %s %d\n", temp_filename, file_size);
    return 1;
}

int get_file_size(char *filename)
{
    FILE *f = fopen(filename, "r");

    if (f == NULL)
    {
        printf("File not found.\n");
        return -1;
    }

    fseek(f, 0L, SEEK_END);
    long int res = ftell(f);
    fclose(f);

    return res;
}

bool file_exists(char *file_name)
{
    struct stat buffer;
    return stat(file_name, &buffer) == 0 ? true : false;
}

void putfile_db(char *buffer)
{
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc = sqlite3_open(SERVER_DB, &db);
    char *sql1;
    char *ErrMsg = 0;
    const char *tail;

    if (rc)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    }

    sqlite3_prepare_v2(db, "SELECT * FROM FILES WHERE FILENAME=?1", 128, &stmt, &tail);
    sqlite3_bind_text(stmt, 1, buffer, -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);

    if (rc != SQLITE_ROW)
    {
        sql1 = sqlite3_mprintf("INSERT INTO FILES (FILENAME) VALUES('%s')", buffer);
        rc = sqlite3_exec(db, sql1, NULL, 0, &ErrMsg);

        if (rc != SQLITE_OK)
        {
            fprintf(stderr, "SQL error: %s\n", ErrMsg);
            sqlite3_free(ErrMsg);
        }
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

void listfiles_db(char *buffer)
{
    sqlite3 *db;
    sqlite3_stmt *res;
    int rc = sqlite3_open(SERVER_DB, &db);
    int count = 0;
    const char *tail;
    char *ErrMsg = 0;

    if (rc)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    }
    if (sqlite3_prepare_v2(db, "SELECT FILENAME FROM FILES", 128, &res, &tail) != SQLITE_OK)
    {
        printf("Can't retrieve data: %s\n", sqlite3_errmsg(db));
    }

    size_t len = strlen(buffer);
    size_t command_len = strlen("LISTFILES");
    memmove(buffer, buffer + command_len, len - command_len + 1);
    while (sqlite3_step(res) == SQLITE_ROW)
    {
        count += 1;
        char *temp_q = (char *)malloc(100 * sizeof(char));
        sprintf(temp_q, "\n(%d) %s\n", count, sqlite3_column_text(res, 0));
        strcat(buffer, temp_q);
        free(temp_q);
    }
    strcat(buffer, "\nENDFILES\n");
    sqlite3_finalize(res);
    sqlite3_close(db);
}

void registered_db(char *client_name, int port_num, int action)
{
    sqlite3 *db;
    int rc = sqlite3_open(SERVER_DB, &db);
    char *sql1;
    char *ErrMsg = 0;

    if (rc)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    }

    if (action == 0)
        sql1 = sqlite3_mprintf("INSERT INTO REGISTERED (PORT, NAME) VALUES('%d', '%s')", port_num, client_name);
    else
        sql1 = sqlite3_mprintf("DELETE FROM REGISTERED WHERE PORT = '%d'", port_num);

    rc = sqlite3_exec(db, sql1, NULL, 0, &ErrMsg);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", ErrMsg);
        sqlite3_free(ErrMsg);
    }
    sqlite3_close(db);
}

char *get_client_name(in_port_t port_number)
{
    char *client_name = malloc(NAME_SIZE);
    sqlite3 *db;
    sqlite3_stmt *stmt;
    const char *tail;
    int rc = sqlite3_open(SERVER_DB, &db);
    char *sql1;
    char *ErrMsg = 0;

    if (rc)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    }
    rc = sqlite3_prepare_v2(db, "SELECT NAME FROM REGISTERED WHERE PORT=?1;", 128, &stmt, &tail);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", ErrMsg);
        sqlite3_free(ErrMsg);
    }
    sqlite3_bind_int(stmt, 1, port_number);
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        strcpy(client_name, sqlite3_column_text(stmt, 0));
    }
    // destroy the object to avoid resource leaks
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return client_name;
}

void listusers_db(char *buffer)
{
    sqlite3 *db;
    sqlite3_stmt *res;
    int rc = sqlite3_open(SERVER_DB, &db);
    int count = 0;
    const char *tail;
    char *ErrMsg = 0;

    if (rc)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    }
    if (sqlite3_prepare_v2(db, "SELECT NAME FROM REGISTERED", 128, &res, &tail) != SQLITE_OK)
    {
        printf("Can't retrieve data: %s\n", sqlite3_errmsg(db));
    }

    size_t len = strlen(buffer);
    size_t command_len = strlen("LISTUSERS");
    memmove(buffer, buffer + command_len, len - command_len + 1);
    while (sqlite3_step(res) == SQLITE_ROW)
    {
        count += 1;
        char *temp_q = (char *)malloc(100 * sizeof(char));
        sprintf(temp_q, "\n(%d) %s\n", count, sqlite3_column_text(res, 0));
        strcat(buffer, temp_q);
        free(temp_q);
    }
    strcat(buffer, "\nENDUSERS\n");
    sqlite3_finalize(res);
    sqlite3_close(db);
}

#endif
