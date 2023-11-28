#include <unistd.h>
#include "util.h"

int main(int argc, char **argv)
{
    int server_fd, new_socket, valread, activitiy, socket_descriptor, max_sd, client_socket[MAX_CLIENT_NUM], questions;
    char *connection_success_message = "[+]Connected successfully (type 'quit' or 'help').\n";

    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    char port[PORT_SIZE] = {0}; // buffer for port size will be stored in
    char buffer[BUFF_SIZE];

    fd_set readfds;

    // initialize all client_socket[] to 0
    for (int i = 0; i < MAX_CLIENT_NUM; i++)
    {
        client_socket[i] = 0;
    }

    printf("[-->]Please enter the port number for server to select:\n");
    fgets(port, 5, stdin);
    setbuf(stdin, NULL); // sets stdin stream buffer NULL

    server_fd = create_and_check_socket();

    // set address family, IP address and port to an int variable
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // IP address
    address.sin_port = htons((short)strtol(port, NULL, 10));

    bind_socket(server_fd, &address);

    // puts server socket in passive mode
    listen_socket(server_fd, 3);
    printf("[+]Server is ready for clients to connect on port number %d.\n", (short)strtol(port, NULL, 10));

    questions = 0;
    create_db();

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
            // setbuf(stdin, NULL); // sets stdin stream buffer NULL
            buffer[BUFF_SIZE] = 0;
            socket_descriptor = client_socket[i];

            if (FD_ISSET(socket_descriptor, &readfds))
            {
                // check if socket is not disconnected, else disconnected
                // if ((valread = read(socket_descriptor, buffer, 1024)) != 0)
                if ((valread = recv(socket_descriptor, buffer, BUFF_SIZE, 0)) != 0)
                {
                    int key = 0;
                    // ASK
                    if (strncmp(buffer, commands[0], strlen(commands[0])) == 0)
                    {
                        key = 1;
                        questions += 1;

                        char question_str[50] = "QUESTION ";
                        char num[12];
                        sprintf(num, "%d", questions);

                        size_t len = strlen(buffer);
                        size_t command_len = strlen(commands[0]);
                        memmove(buffer, buffer + command_len, len - command_len + 1);
                        ask_db(buffer);

                        strcat(question_str, num);
                        strcat(question_str, ": ");
                        strcat(question_str, buffer);
                        strcpy(buffer, question_str);
                    }
                    // ANSWER
                    else if (strncmp(buffer, commands[1], strlen(commands[1])) == 0)
                    {
                        key = 2;
                        size_t len = strlen(buffer);
                        size_t command_len = strlen(commands[1]);

                        char answer_str[10] = "ANSWER ";
                        char num = buffer[8];

                        memmove(buffer, buffer + command_len + 2, len - command_len + 1);
                        answer_db(buffer, num);

                        strncat(answer_str, &num, 1);
                        strcat(answer_str, ": ");
                        strcat(answer_str, buffer);
                        strcpy(buffer, answer_str);
                    }
                    // LISTQUESTIONS
                    else if (strncmp(buffer, commands[2], strlen(commands[2])) == 0)
                    {
                        listquestions_db(buffer, questions);
                        send(socket_descriptor, buffer, strlen(buffer) + 1, 0);
                        continue;
                    }
                    // PUTFILE
                    else if (strncmp(buffer, commands[3], strlen(commands[3])) == 0)
                    {
                        char path[BUFF_SIZE];
                        char temp_filename[BUFF_SIZE];
                        const char seperator = ' ';
                        char *filename = strchr(buffer, seperator);
                        *filename = '\0';

                        strcpy(temp_filename, filename + 1);
                        strcpy(path, "./remote/");
                        strcat(path, filename + 1);
                        receive_file(socket_descriptor, path, path);
                        
                        putfile_db(temp_filename);
                        continue;
                    }
                    // GETFILE
                    else if (strncmp(buffer, commands[4], strlen(commands[4])) == 0)
                    {
                        // send(socket_descriptor, buffer, strlen(buffer) + 1, 0);
                        char path[BUFF_SIZE];
                        char temp[BUFF_SIZE];
                        strcpy(temp, buffer);

                        const char seperator = ' ';
                        char *filename = strchr(buffer, seperator);
                        *filename = '\0';
                        strcpy(path, "./remote/");
                        strcat(path, filename + 1);

                        if (!file_exists(path))
                        {
                            send(socket_descriptor, "File not found.", strlen("File not found.") + 1, 0);
                            continue;
                        }
                        else
                        {
                            send(socket_descriptor, temp, strlen(temp) + 1, 0);
                        }

                        send_file(socket_descriptor, path, path);
                        continue;
                    }
                    // LISTFILES
                    else if (strncmp(buffer, commands[5], strlen(commands[5])) == 0)
                    {
                        //char buffer[BUFF_SIZE];
                        listfiles_db(buffer);
                        send(socket_descriptor, buffer, strlen(buffer) + 1, 0);
                        continue;
                    }

                    if (key != 0)
                    {
                        printf("%s\n", buffer);
                    }
                    else
                    {
                        printf("Client[%d]: %s\n", socket_descriptor - 4, buffer);
                    }

                    for (int j = 0; j < MAX_CLIENT_NUM; j++)
                    {
                        if (client_socket[j] != socket_descriptor)
                        {
                            if (key != 0)
                            {
                                send(client_socket[j], buffer, strlen(buffer) + 1, 0);
                            }
                            else
                            {
                                send_other_client(buffer, socket_descriptor, client_socket[j]);
                            }
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
    File Management:
    => PUTFILE, LISTFILES, GETFILE
    a new table will be needed for this stage, called FILES
    | id | file_name |

    PUTFILE cv.pdf 2780 => upload files to the server, (byte option must be tested) [FINISHED]
    LISTFILES => lists uploaded files, (works same as LISTQUESTIONS)
    GETFILE -{id} => downloads a file from server
*/

/*
    TODO:
        BUG =>
        fix '' usage in when asking questions

        add constants for array sizes
        delete unnecessery comments and iff add comment if it's necessery

        learn more about file transfer and make it without exceeding 1024KB packet size
            => so if a file is larger than the packet, multiple packets should be sent
        try edge cases for PUTFILE command
*/