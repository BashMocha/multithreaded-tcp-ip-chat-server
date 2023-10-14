#include <unistd.h>
#include <stdbool.h>
#include "util.h"


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
    setbuf(stdin, NULL);    // sets stdin stream buffer NULL 

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

    for (;;)
    {
        // if its the first time client connects to server, gets a message from server,
        // indicates that connection is successful
        if (first_connection)
        {
            if ((valread = read(client_fd, buffer, 1024)) != 0)
            {
                printf("%s", buffer);
            }
            first_connection = false;
        }

        printf("Message: ");
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = 0;
        
        // break loop for quitting
        if (strcmp(buffer, "quit") == 0)
            break;
        
        send(client_fd, buffer, strlen(buffer) + 1, 0);
    }

    close(client_fd);
    return 0;
}
