#include <unistd.h>
#include "util.h"

int main(int argc, char **argv)
{
    int server_fd, new_socket, valread, activitiy, socket_descriptor, max_sd, client_socket[MAX_CLIENT_NUM];
    char *connection_success_message = "[+]Connected successfully (type 'quit').\n";

    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    char port[PORT_SIZE] = {0}; // buffer for port size will be stored in
    char buffer[BUFF_SIZE];     // data buffer of 1K

    // set of socket descriptors
    fd_set readfds;

    // initialize all client_socket[] to 0
    for (int i = 0; i < MAX_CLIENT_NUM; i++)
    {
        client_socket[i] = 0;
    }

    printf("[-->]Please enter the port number for server to select:\n");
    fgets(port, 5, stdin);
    setbuf(stdin, NULL); // sets stdin stream buffer NULL
    create_db();
    insert_db();
    // creating a TCP socket in IP protocol
    // this is the default (master) socket
    server_fd = create_and_check_socket();

    // set address family, IP address and port to an int variable
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // IP address
    address.sin_port = htons((short)strtol(port, NULL, 10));

    // bind socket to address and port
    bind_socket(server_fd, &address);

    // puts server socket in passive mode
    listen_socket(server_fd, 3);
    printf("[+]Server is ready for clients to connect on port number %d.\n", (short)strtol(port, NULL, 10));

    for (;;)
    {
        // clear the socket set
        FD_ZERO(&readfds);

        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        for (int i = 0; i < MAX_CLIENT_NUM; i++)
        {
            socket_descriptor = client_socket[i];

            // if valid socket descriptor then add to read list
            if (socket_descriptor > 0)
                FD_SET(socket_descriptor, &readfds);

            // highest file descriptor number, need it for later for select() function
            if (socket_descriptor > max_sd)
                max_sd = socket_descriptor;
        }

        // waits for an activity on one of the sockets,
        // set timeout = NULL, might wait indefinitely
        activitiy = select(max_sd + 1, &readfds, NULL, NULL, &tv);

        // checks if any errors occurs or timeout expires during select system call, exit program
        if (((activitiy == 0) || (activitiy < 0)) && (errno != EINTR))
        {
            if (activitiy == 0)
                printf("[!]Server is shutting down due to inactivity.\n");
            else
                printf("Select failed.\n");
            exit(EXIT_FAILURE);
        }

        // reset timeout due to activity
        tv.tv_sec = TIMEOUT;

        // if something occurs on default socket, then its an incoming connections
        if (FD_ISSET(server_fd, &readfds))
        {
            // accepts the first connection request, creates new socket and, the connection is established between server and client
            new_socket = accept_new_socket(server_fd, &address, &addrlen);
            printf("[-->]New connection, socket_fd: %d, IP: %s, port: %d.\n", new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

            // send an connection message that indicates connection is successful to client
            if (send(new_socket, connection_success_message, strlen(connection_success_message) + 1, 0) != strlen(connection_success_message) + 1)
            {
                perror("[-]Message couldn't send.\n");
            }
            puts("[+]Message sent successfully.\n");

            // add new_socket into array of sockets if the position is empty
            for (int i = 0; i < MAX_CLIENT_NUM; i++)
            {
                if (client_socket[i] == 0)
                {
                    client_socket[i] = new_socket;
                    break;
                }
            }
        }

        for (int i = 0; i < MAX_CLIENT_NUM; i++)
        {
            socket_descriptor = client_socket[i];

            if (FD_ISSET(socket_descriptor, &readfds))
            {
                // check if socket is not disconnected, else disconnected
                // if ((valread = read(socket_descriptor, buffer, 1024)) != 0)
                if ((valread = recv(socket_descriptor, buffer, BUFF_SIZE, 0)) != 0)
                {
                    printf("Client[%d]: %s\n", socket_descriptor - 4, buffer);
                    // send(socket_descriptor , buffer, strlen(buffer) + 1, 0 );

                    for (int j = 0; j < MAX_CLIENT_NUM; j++)
                    {
                        if (client_socket[j] != socket_descriptor)
                        {
                            send_other_client(buffer, socket_descriptor, client_socket[j]);
                        }
                    }
                }
                else
                {
                    // print host information and close its file descriptor
                    getpeername(socket_descriptor, (struct sockaddr *)&address, (socklen_t *)&addrlen);
                    printf("[-]Host disconnected, IP: %s, port: %d.\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

                    close(socket_descriptor);
                    client_socket[i] = 0;
                }
            }
        }
    }

    close(new_socket);
    shutdown(server_fd, SHUT_RDWR);
    return 0;
}

/*
    Management of questions and answers:
    The component supports 3 different commands: "ASK", "ANSWER", "LISTQUESTIONS":
    (use SQLite in order to provide data persistency)
    
    ASK ...
    ANSWER ...
    LISTQUESTIONS ...

    client 0:
    ASK who was the first king of Portugal?
    
    ANSWER 1: Afonso Henriques

    ----

    client 1:
    ASK 1: who was the first king of Portugal?
    ANSWER Afonso Henriques

*/