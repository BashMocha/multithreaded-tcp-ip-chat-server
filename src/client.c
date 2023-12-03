#include <unistd.h>
#include <stdbool.h>
#include "util.h"
#include <pthread.h>

int main(int argc, char **argv)
{
    int status, client_fd, valread;
    struct sockaddr_in serv_addr;
    char buffer[BUFF_SIZE] = {0}; // data buffer of 1K
    char client_name[NAME_SIZE];

    bool first_connection = true;
    client_fd = create_and_check_socket();

    char port[PORT_SIZE] = {0}; // buffer for port size will be stored in

    printf("[-->]Type the port address you want to connect:");
    fgets(port, 5, stdin);
    setbuf(stdin, NULL); // sets stdin stream buffer NULL

    struct hostent *server = gethostbyname("localhost");

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons((short)strtol(port, NULL, 10));
    serv_addr.sin_addr.s_addr = *server->h_addr;

    printf("[-->]Please type your name:");
    fgets(client_name, NAME_SIZE, stdin);
    strtok(client_name, "\n");
    setbuf(stdin, NULL);

    // connects the socket referred to by the file descriptor sockfd to the address specified by addr
    if ((status = connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0)
    {
        printf("Connection failed.\n");
        exit(EXIT_FAILURE);
    }
    send(client_fd, client_name, strlen(client_name) + 1, 0);

    pthread_t id_read = message_read_thread(client_fd);
    pthread_t id_send = message_send_thread(client_fd);
    pthread_join(id_read, 0);
    pthread_join(id_send, 0);

    close(client_fd);
    return 0;
}
