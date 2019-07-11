#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <fcntl.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <errno.h>

#include "data_base.h"

#define PORT "3490"
#define BASE_PATH "/home/matrin/code/CRUD/server/data.json"
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
    fseek(fp, SEEK_SET, 0);
    return fprintf(fp, "%d", num);
}

int main()
{
    fprintf(stdout, "loading database ...\n");
    json_error_t error;
    json_t *database = json_load_file(BASE_PATH, 0, &error); // sergmentation fault ?????
    if (!database)
    {
        fprintf(stderr, "database loading failed: %s\n", error.text);
        return 1;
    }
    fprintf(stdout, "the database was successfully loaded\n");

    FILE *fp = fopen("ID", "rw");
    if (!fp)
    {
        fprintf(stderr, "failed open ID\n");
        return 2;
    }
    int ID;
    fscanf(fp, "%d", &ID);

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

    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1)
    {
        fprintf(stderr, "bind failed.\n");
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

    while (1)
    {
        static char buffer[BUFFER_SIZE];
        int new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &slen);
        set_nonblock(new_fd);

        fprintf(stdout, "new user was connected");

        int nCommand = 0;
        int size = recv(new_fd, &nCommand, sizeof(int), MSG_NOSIGNAL);
        if (size != sizeof(int))
        {
            send(new_fd, "error", sizeof("error"), 0);
            shutdown(new_fd, SHUT_RDWR);
            continue;
        }
        else
        {
            for (int i = 0; i < nCommand; i++)
            {
                int size = recv(new_fd, buffer, sizeof(buffer), MSG_NOSIGNAL);
                if (size == -1 && errno == EAGAIN)
                {
                    size = recv(new_fd, buffer, sizeof(buffer), 0);
                } // ?????????
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
                json_t *params = json_object_get(object, "params");
                if (!strcmp(command_text, "create"))
                {
                    if (create(database, params, ID) == -1)
                        send(new_fd, "error", sizeof("error"), 0);
                    else
                    {
                        char respond[128];
                        snprintf(respond, 128, "messageID is %d", ID++);
                        send(new_fd, respond, strlen(respond), 0);
                        json_dump_file(database, BASE_PATH, 0);

                        write_num(fp, ID);
                    }
                }
                else if (!strcmp(command_text, "read"))
                {
                    json_t *msgIDs = json_object_get(params, "messageID");
                    if (json_is_integer(msgIDs))
                    {
                        int status = read(database, json_integer_value(msgIDs), buffer, 1024);
                        if (status == -1)
                        {
                            send(new_fd, "read error\n", sizeof("read error\n"), 0);
                            json_decref(msgIDs);
                            continue;
                        }
                        else if (status == 1)
                            send(new_fd, "not a whole file was read:\n", sizeof("not a whole file was read:\n"), 0);
                        send(new_fd, buffer, strlen(buffer), 0);
                        json_decref(msgIDs);
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
                            read(database, json_integer_value(value), buffer, 1024);
                            send(new_fd, buffer, strlen(buffer), 0);
                        }
                    }
                }
                else if (!strcmp(command_text, "update"))
                {
                    json_t *msgID = json_object_get(params, "messageID");
                    if (!msgID && !json_is_integer(msgID))
                    {
                        send(new_fd, "error getting messageID\n", sizeof("error getting messageID\n"), 0);
                        json_decref(msgID);
                        continue;
                    }
                    json_object_del(params, "messageID");
                    int status = update(database, params, json_integer_value(msgID));
                    char response[128];
                    snprintf(response, 128, "updating of message %lld was %s", json_integer_value(msgID), (status == -1 ? "failed" : "succeed"));
                    send(new_fd, response, strlen(response), 0);
                }
            }
        }
        shutdown(new_fd, SHUT_RDWR);
    }

    json_decref(database);
    return 0;
}