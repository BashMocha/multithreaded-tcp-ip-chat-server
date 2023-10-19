#include <unistd.h>
#include <stdbool.h>
#include "util.h"
#include <fcntl.h>

int main(int argc, char **argv)
{
    int status, client_fd, valread;
    struct sockaddr_in serv_addr;

    char message[100];
    char buffer[1025] = {0}; // data buffer of 1K

    bool first_connection = true;
    client_fd = create_and_check_socket();

    // char* port;
    char port[PORT_SIZE] = {0}; // buffer for port size will be stored in

    printf("Type the port address you want to connect:");
    fgets(port, 5, stdin);
    setbuf(stdin, NULL); // sets stdin stream buffer NULL

    struct hostent *server = gethostbyname("localhost");

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons((short)strtol(port, NULL, 10));
    serv_addr.sin_addr.s_addr = *server->h_addr;

    // connects the socket referred to by the file descriptor sockfd to the address specified by addr
    if ((status = connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0)
    {
        printf("Connection failed.\n");
        exit(EXIT_FAILURE);
    }


    fd_set all_set, r_set;
    int maxfd = client_fd + 1;
    FD_ZERO(&all_set);
    FD_SET(STDIN_FILENO, &all_set);
    FD_SET(client_fd, &all_set);
    r_set = all_set;
    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;

    //printf("Message: ");
    for (;;)
    {
        if (first_connection)
        {
            if ((valread = read(client_fd, buffer, 1024)) != 0)
            {
                printf("%s", buffer);
            }
            first_connection = false;
        }

        r_set = all_set;
        select(maxfd, &r_set, NULL, NULL, &tv);

        if (FD_ISSET(STDIN_FILENO, &r_set))
        {
            //printf("Message: ");
            fgets(buffer, sizeof(buffer), stdin);
            buffer[strcspn(buffer, "\n")] = 0;

            // break loop for quitting
            if (strcmp(buffer, "quit") == 0)
                break;

            send(client_fd, buffer, strlen(buffer) + 1, 0);
        }

        // if smth occurs on default socket, accept and create new socket
        if (FD_ISSET(client_fd, &r_set))
        {
            if ((valread = read(client_fd, buffer, 1024)) != 0)
            {   
                printf("%s\n", buffer);
            }
        }

    }

    close(client_fd);
    return 0;
}
