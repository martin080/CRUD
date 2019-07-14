#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/poll.h>

#include <fcntl.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <errno.h>

#include "data_base.h"

#define PORT "3490"
#define BASE_PATH "/home/matrin/code/CRUD/server/data.json"
#define MAX_CONNECTIONS 10
#define BUFFER_SIZE 1024

int set_nonblock(int fd)
{
    int flags;
#if defined(O_NONBLOCK)
    if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
        flags = 0;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
    flags = 1;
    return (ioctrl(fd, FIOBNIO, &flags));
#endif
}

int write_num(FILE *fp, int num)
{
    fseek(fp, 0, SEEK_SET);
    return fprintf(fp,"%d", num);
}

int main()
{
    fprintf(stdout, "loading database ...\n");
    json_error_t error;
    json_t *database = json_load_file(BASE_PATH, 0, &error);
    if (!database)
    {
        fprintf(stderr, "database loading failed: %s\n", error.text);
        return 1;
    }
    fprintf(stdout, "the database was successfully loaded\n");

    FILE *id_file = fopen("ID", "r+");
    int ID; fscanf(id_file, "%d", &ID);

    int sockfd;
    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int status = getaddrinfo(NULL, PORT, &hints, &res);
    if (status == -1)
    {
        fprintf(stderr, "getaddrinfo failed: %s\n", gai_strerror(status));
        exit(0);
    };

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1)
    {
        fprintf(stderr, "socket failed.\n");
        exit(1);
    }
    set_nonblock(sockfd);

    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1)
    {
        fprintf(stderr, "bind failed: %s\n",strerror(errno));
        exit(2);
    }

    if (listen(sockfd, SOMAXCONN) == -1)
    {
        fprintf(stderr, "listen failed.\n");
        exit(3);
    }

    freeaddrinfo(res);

    struct sockaddr_storage their_addr;
    socklen_t slen = sizeof(their_addr);

    struct pollfd pfd[MAX_CONNECTIONS + 1];
    pfd[0].fd = sockfd;
    pfd[0].events = POLLIN | POLLHUP;
    for (int i = 1; i < MAX_CONNECTIONS + 1; i++)
        pfd[i].fd = -1;
    int cur_connections = 0;

    while (1)
    {
        static char buffer[BUFFER_SIZE], response[128];
        int ret = poll(pfd, MAX_CONNECTIONS + 1, -1);

        if (ret == -1)
        {
            fprintf(stderr, "poll failed\n");
            continue;
        }

        if (pfd[0].revents & POLLIN)
        {
            int new_fd = 0;
            while (new_fd != -1)
            {
                new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &slen);
                if (new_fd < 0)
                    continue;
                printf("  new connection on socket %d\n", new_fd);

                set_nonblock(new_fd);

                int i = 1;
                for (; i < MAX_CONNECTIONS + 1; i++)
                {
                    if (pfd[i].fd == -1)
                    {
                        pfd[i].fd = new_fd;
                        pfd[i].events = POLLIN;
                        cur_connections++;
                        break;
                    }
                }
                if (i == MAX_CONNECTIONS + 1)
                {
                    strcpy(response, "server is unable to serve you, please try later\n");
                    send(new_fd, response, strlen(response), 0);
                    shutdown(new_fd, SHUT_RDWR);
                }
            }
        }
        for (int i = 1; i < cur_connections + 1; i++)
        {
            if (pfd[i].revents & POLLHUP)
            {
                printf("  connection on socket %d was shut\n", pfd[i].fd);

                close(pfd[i].fd);
                for (int j = i; j < cur_connections; j++)
                {
                    pfd[j].fd = pfd[j + 1].fd;
                    pfd[j].revents = pfd[j + 1].revents;
                    pfd[j].events = pfd[j+1].events;
                }
                cur_connections--;
                continue;
            }

            if (!(pfd[i].revents & POLLIN))
                continue;

            int new_fd = pfd[i].fd;
            pfd[i].revents = 0;

            printf("  reading from socket %d ... \n", new_fd);

            int size = recv(new_fd, buffer, sizeof(buffer), MSG_NOSIGNAL);
            if (size == -1 && errno != EAGAIN)
            {
                printf("  recv() on socket %d failed\n", pfd[i].fd);
                continue;
            } 

            buffer[size++] = '\0';
            json_error_t error;
            json_t *object = json_loads(buffer, 0, &error);
            if (!object)
            {
                send(new_fd, error.text, sizeof(error.text), 0);
                continue;
            }

            json_t *command = json_object_get(object, "command");
            const char *command_text = json_string_value(command);

            printf("  processing the request on socket %d ... \n", new_fd);

            json_t *params = json_object_get(object, "params");
            if (!strcmp(command_text, "create")) // command == create
            {
                if (create(database, params, ID) == -1)
                {
                    snprintf(response, 128, "Error creating new data, please try again");
                    send(new_fd, response, strlen(response), 0);
                }
                else
                {
                    snprintf(response, 128, "Data was loaded successfully, messageID is %d", ID++);
                    send(new_fd, response, strlen(response), 0);
                    json_dump_file(database, BASE_PATH, 0);

                    write_num(id_file, ID);
                }
            }
            else if (!strcmp(command_text, "read")) //command == read
            {
                json_t *msgIDs = json_object_get(params, "messageID");

                if (json_is_integer(msgIDs))
                {
                    int id = json_integer_value(msgIDs);
                    int status = read_object(database, id, buffer, BUFFER_SIZE - 1);
                    if (status == -1)
                    {
                        snprintf(response, 128, "Error reading data, messageID = %d", id);
                        send(new_fd, response, strlen(response), 0);
                        json_decref(msgIDs);
                        continue;
                    }
                    else if (status == 0)
                    {
                        snprintf(response, 128, "not a whole data was from message %d read:\n", id);
                        send(new_fd, response, strlen(response), 0);
                    }
                    send(new_fd, buffer, strlen(buffer), 0);
                    json_decref(msgIDs);
                }
                else if (json_is_array(msgIDs))
                {
                    json_t *value;
                    size_t index;
                    size_t was_read = 0;
                    json_array_foreach(msgIDs, index, value)
                    {
                        if (!json_is_integer(value))
                            continue;
                        int id = json_integer_value(value);
                        int size = read_object(database, id, buffer + was_read, BUFFER_SIZE - was_read);
                        if (size == -1)
                        {
                            snprintf(response, 128, "Error reading data, messageID = %d (command %d)\n", id, i + 1);
                            send(new_fd, response, strlen(response), 0);
                            json_decref(msgIDs);
                            continue;
                        }
                        else if (size == 0)
                        {
                            snprintf(response, 128, "not a whole data was from message %d read:\n", id);
                            send(new_fd, response, strlen(response), 0);
                        }
                        was_read += size;
                        if (was_read < 1023)
                        {
                            buffer[was_read++] = '\n';
                            buffer[was_read] = '\0';
                        }
                        else 
                        {
                            send(new_fd, buffer, strlen(buffer), 0);
                            was_read = 0;
                        }
                    }
                    send(new_fd, buffer, strlen(buffer), 0);
                    json_decref(msgIDs);
                }
                else
                {
                    snprintf(response, 128, "Wrong messageID format in command %d", i + 1);
                    send(new_fd, response, strlen(response), 0);
                    json_decref(msgIDs);
                }
            }
            else if (!strcmp(command_text, "update")) // command == update
            {
                json_t *msgID = json_object_get(params, "messageID");
                if (!msgID || !json_is_integer(msgID))
                {
                    printf("error processing request on socket %d\n", new_fd);

                    snprintf(response, 128, "Wrong messageID format in command %d", i + 1);
                    send(new_fd, response, strlen(response), 0);
                    json_decref(msgID);
                    continue;
                }

                int id = json_integer_value(msgID);

                json_object_del(params, "messageID");

                int status = update(database, params, id);
                char response[128];
                snprintf(response, 128, "updating of message %d was %s", id, (status == -1 ? "failed" : "succeed"));
                if (status != -1)
                    json_dump_file(database, BASE_PATH, 0);
                send(new_fd, response, strlen(response), 0);
            }
            else if (!strcmp(command_text, "delete")) // command == delete
            {
                json_t *msgIDs = json_object_get(params, "messageID");
                if (json_is_integer(msgIDs))
                {
                    int id = json_integer_value(msgIDs);
                    int status = delete_object(database, id);
                    if (status == -1)
                    {
                        printf("error processing request on socket %d\n", new_fd);

                        snprintf(response, 128, "Error delete data, messageID = %d", id);
                        send(new_fd, response, strlen(response), 0);
                        json_decref(msgIDs);
                        continue;
                    }
                    json_dump_file(database, BASE_PATH, 0);
                    snprintf(response, 128, "message %d was deleted (command %d)", id, i + 1);
                    json_decref(msgIDs);

                    send(new_fd, response, strlen(response), 0);
                    continue;
                }
                else if (json_is_array(msgIDs))
                {
                    json_t *value;
                    size_t index;
                    json_array_foreach(msgIDs, index, value)
                    {
                        if (!json_is_integer(value))
                            continue;
                        int id = json_integer_value(value);
                        int status = delete_object(database, id);
                        if (status == -1)
                            snprintf(response, 128, "delete of message %d failed (command %d)", id, i + 1);
                        else
                        {
                            snprintf(response, 128, "message %d was deleted (command %d)", id, i + 1);
                            json_dump_file(database, BASE_PATH, 0);
                        }
                        send(new_fd, response, strlen(response), 0);
                    }
                    json_decref(msgIDs);
                }
                else
                {
                    json_decref(msgIDs);

                    snprintf(response, 128, "Wrong messageID format in command %d", i + 1);
                    send(new_fd, response, strlen(response), 0);
                }
            }
            json_decref(command);
            json_decref(params);

            printf("  request was processed \n");
        }
    }
    json_decref(database);
    return 0;
}