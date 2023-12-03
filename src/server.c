#include <unistd.h>
#include "util.h"

int main(int argc, char **argv)
{
    int server_fd, new_socket, valread, activitiy, socket_descriptor, max_sd, client_socket[MAX_CLIENT_NUM], questions;
    char *connection_success_message = "[+]Connected successfully (type 'quit' or 'help').\n";
    char client_name[NAME_SIZE];
    bool flag;

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

    printf("[-->]Please enter the port number for server to select:");
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
            recv(new_socket, client_name, sizeof(client_name), 0);
            registered_db(client_name, ntohs(address.sin_port), 0);

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
            buffer[BUFF_SIZE] = 0;
            socket_descriptor = client_socket[i];

            if (FD_ISSET(socket_descriptor, &readfds))
            {
                getpeername(socket_descriptor, (struct sockaddr *)&address, (socklen_t *)&addrlen);

                //  check if socket is disconnected, else disconnected
                if ((valread = recv(socket_descriptor, buffer, BUFF_SIZE, 0)) != 0)
                {
                    // print client message if it's true
                    // else print answer and question commands
                    flag = false;

                    // ASK command
                    if (strncmp(buffer, commands[0], strlen(commands[0])) == 0)
                    {
                        flag = true;
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
                    // ANSWER command
                    else if (strncmp(buffer, commands[1], strlen(commands[1])) == 0)
                    {
                        flag = true;
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
                    // LISTQUESTIONS command
                    else if (strncmp(buffer, commands[2], strlen(commands[2])) == 0)
                    {
                        listquestions_db(buffer);
                        send(socket_descriptor, buffer, strlen(buffer) + 1, 0);
                        continue;
                    }
                    // PUTFILE command
                    else if (strncmp(buffer, commands[3], strlen(commands[3])) == 0)
                    {
                        int check;
                        char path[BUFF_SIZE];
                        char temp_filename[BUFF_SIZE];
                        const char seperator = ' ';
                        char *filename = strchr(buffer, seperator);
                        *filename = '\0';

                        strcpy(temp_filename, filename + 1);
                        strcpy(path, "./remote/");
                        strcat(path, filename + 1);

                        check = receive_file(socket_descriptor, path);

                        if (check == -1)
                        {
                            printf("File could not received.\n");
                        }
                        else
                        {
                            putfile_db(temp_filename);
                        }

                        continue;
                    }
                    // GETFILE command
                    else if (strncmp(buffer, commands[4], strlen(commands[4])) == 0)
                    {
                        int check;
                        char path[BUFF_SIZE];
                        char temp_buffer[BUFF_SIZE];
                        strcpy(temp_buffer, buffer);

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
                            send(socket_descriptor, temp_buffer, strlen(temp_buffer) + 1, 0);
                        }

                        check = send_file(socket_descriptor, path);
                        if (check == -1)
                        {
                            printf("File could not send.\n");
                        }

                        continue;
                    }
                    // LISTFILES command
                    else if (strncmp(buffer, commands[5], strlen(commands[5])) == 0)
                    {
                        listfiles_db(buffer);
                        send(socket_descriptor, buffer, strlen(buffer) + 1, 0);
                        continue;
                    }
                    // LISTUSERS command
                    else if (strncmp(buffer, commands[6], strlen(commands[6])) == 0)
                    {
                        listusers_db(buffer);
                        send(socket_descriptor, buffer, strlen(buffer) + 1, 0);
                        continue;
                    }

                    char *client_name = get_client_name(ntohs(address.sin_port));
                    client_name = get_client_name(ntohs(address.sin_port));

                    // print QUESTION and ANSWER
                    if (flag)
                    {
                        printf("%s\n", buffer);
                    }
                    // print client message
                    else
                    {
                        printf("%s: %s\n", client_name, buffer);
                    }

                    for (int j = 0; j < MAX_CLIENT_NUM; j++)
                    {
                        if (client_socket[j] != socket_descriptor)
                        {
                            // send question and answer
                            if (flag)
                            {
                                send(client_socket[j], buffer, strlen(buffer) + 1, 0);
                            }
                            // broadcast client message
                            else
                            {
                                char temp_name[NAME_SIZE];
                                strcpy(temp_name, client_name);
                                strcat(temp_name, ": ");
                                strcat(temp_name, buffer);
                                send(client_socket[j], temp_name, strlen(temp_name) + 1, 0);
                            }
                        }
                    }
                    free(client_name);
                }
                else
                {
                    // print host information and close its file descriptor
                    printf("[-]Host disconnected, IP: %s, port: %d.\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                    registered_db(NULL, ntohs(address.sin_port), 1);
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
    TODO:
        BUG =>
        fix '' usage in when asking questions

        add constants for array sizes
        delete unnecessery comments and iff add comment if it's necessery

        refactor methods
        fix printing file names after PUTFILE and GETFILE
*/