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
#define BASE_PATH "data.json"
#define MAX_CONNECTIONS 10
#define BUFFER_SIZE 2048

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
    return fprintf(fp, "%d", num);
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
    if (!id_file)
    {
        fprintf(stderr, "ID file open failed\n");
        return 2;
    }
    int ID;
    fscanf(id_file, "%d", &ID);
    ID++;

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
        fprintf(stderr, "bind failed: %s\n", strerror(errno));
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
    pfd[0].events = POLLIN;
    for (int i = 1; i < MAX_CONNECTIONS + 1; i++)
        pfd[i].fd = -1;
    int cur_connections = 0;

    while (1)
    {
        static char buffer[BUFFER_SIZE], response[128];
        int ret = poll(pfd, cur_connections + 1, -1);

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

                set_nonblock(new_fd);

                if (cur_connections < MAX_CONNECTIONS)
                {
                    pfd[cur_connections + 1].fd = new_fd;
                    pfd[cur_connections + 1].events = POLLIN | POLLHUP;
                    cur_connections++;

                    printf("  new connection on socket %d\n", new_fd);

                    continue;
                }

                strcpy(response, "server is unable to serve you, please try later\n");
                send(new_fd, response, strlen(response), 0);
                close(new_fd);
            }
        }
        for (int i = 1; i < cur_connections + 1; i++)
        {
            if (pfd[i].revents & POLLHUP)
            {
                printf("  connection was lost, socket %d\n", pfd[i].fd);
                pfd[i].revents = 0;

                close(pfd[i].fd);
                for (int j = i; j < cur_connections; j++)
                {
                    pfd[j].fd = pfd[j + 1].fd;
                    pfd[j].events = pfd[j + 1].events;
                    pfd[j].revents = pfd[j + 1].revents;
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
            if (size == -1)
            {
                printf("recv() on socket %d failed\n", new_fd);
                continue;
            }

            buffer[size] = '\0';

            printf("  client request: %s \n", buffer);

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
                    if (send(new_fd, response, strlen(response), 0) == -1)
                        printf("send() on socket %d failed", new_fd);
                }
                else
                {
                    snprintf(response, 128, "Data was loaded successfully, messageID is %d", ID);
                    if (send(new_fd, response, strlen(response), 0) == -1)
                        printf("send() on socket %d failed", new_fd);

                    json_dump_file(database, BASE_PATH, 0);

                    write_num(id_file, ++ID);
                }
            }
            else if (!strcmp(command_text, "read")) //command == read
            {
                json_t *msgIDs = json_object_get(params, "messageID");

                if (json_is_integer(msgIDs))
                {
                    int id = json_integer_value(msgIDs);
                    ssize_t was_read = 0, size;
                    size = read_object(database, id, buffer, BUFFER_SIZE - 2, was_read);
                    was_read += size;
                    if (size == -1)
                    {
                        snprintf(response, 128, "Error reading data, messageID = %d", id);
                        send(new_fd, response, strlen(response), 0);
                        json_decref(msgIDs);
                        continue;
                    }
                    while (size == BUFFER_SIZE - 2)
                    {
                        buffer[size++] = '\n';
                        buffer[size] = 0;
                        if (send(new_fd, buffer, strlen(buffer), 0) == -1)
                            printf("send() on socket %d failed", new_fd);

                        printf("  data \"%s\" is sending on socket %d\n", buffer, pfd[i].fd);

                        size = read_object(database, id, buffer, BUFFER_SIZE - 2, was_read);
                        was_read += size;
                    }

                    buffer[size++] = '\n';
                    buffer[size] = '\0';
                    if (send(new_fd, buffer, strlen(buffer), 0) == -1)
                        printf("send() on socket %d failed", new_fd);

                    printf("  data \"%s\" is sending on socket %d\n", buffer, pfd[i].fd);

                    json_decref(msgIDs);
                }
                else if (json_is_array(msgIDs))
                {
                    json_t *value;
                    size_t index;
                    ssize_t was_read = 0;
                    json_array_foreach(msgIDs, index, value)
                    {
                        if (!json_is_integer(value))
                            continue;
                        int id = json_integer_value(value);
                        ssize_t size = read_object(database, id, buffer + was_read, BUFFER_SIZE - 2, 0);
                        if (size == -1)
                        {
                            snprintf(response, 128, "Error reading data, messageID = %d\n", id);
                            if (send(new_fd, response, strlen(response), 0) == -1)
                                printf("send() on socket %d failed", new_fd);
                        }

                        was_read += size;
                        if (was_read < BUFFER_SIZE - 2)
                        {
                            buffer[was_read++] = '\n';
                            buffer[was_read] = '\0';
                        }
                        else
                        {
                            if (send(new_fd, buffer, strlen(buffer), 0) == -1)
                                printf("send() on socket %d failed", new_fd);

                            was_read = 0;
                        }
                    }
                    if (send(new_fd, buffer, strlen(buffer), 0))
                        printf("send() on socket %d failed", new_fd);

                    json_decref(msgIDs);
                }
                else
                {
                    snprintf(response, 128, "Wrong messageID format in command %d", i + 1);
                    if (send(new_fd, response, strlen(response), 0) == -1)
                        printf("send() on socket %d failed", new_fd);
                        
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
                    if (send(new_fd, response, strlen(response), 0) == -1)
                        printf("send() on socket %d failed", new_fd);

                    json_decref(msgID);
                    continue;
                }

                int id = json_integer_value(msgID);

                json_object_del(params, "messageID");

                int status = update(database, params, id);
                snprintf(response, 128, "updating of message %d was %s", id, (status == -1 ? "failed" : "succeed"));
                if (status != -1)
                    json_dump_file(database, BASE_PATH, 0);
                if (send(new_fd, response, strlen(response), 0) == -1)
                    printf("send() on socket %d failed", new_fd);
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
                        if (send(new_fd, response, strlen(response), 0) == -1)
                            printf("send() on socket %d failed", new_fd);

                        json_decref(msgIDs);
                        continue;
                    }
                    json_dump_file(database, BASE_PATH, 0);
                    snprintf(response, 128, "message %d was deleted (command %d)", id, i + 1);
                    json_decref(msgIDs);

                    if (send(new_fd, response, strlen(response), 0) == -1)
                        printf("send() on socket %d failed", new_fd);

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
                        if (send(new_fd, response, strlen(response), 0))
                            printf("send() on socket %d failed", new_fd);
                    }
                    json_decref(msgIDs);
                }
                else
                {
                    json_decref(msgIDs);

                    snprintf(response, 128, "Wrong messageID format in command %d", i + 1);
                    if (send(new_fd, response, strlen(response), 0) == -1)
                        printf("send() on socket %d failed", new_fd);
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