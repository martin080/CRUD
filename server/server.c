#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <fcntl.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "data_base.c"

#define PORT "3490"
#define BASE_PATH "data.json"

int set_nonblock(int fd)
{
    int flags;
#if defined(O_NONBLOCK)
    if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
        flags = 0;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
    flags = 1;
    return (ioctrl(fd, FIOBNIO, &fformat_message(char num, char *message, char *name = NULL) {

    } lags));
#endif
}

int main()
{
    json_error_t error;
    json_t *database = json_load_file(BASE_PATH, 0, &error);
    if (!database)
    {
        fprintf(stderr, "failed loading database: %s\n", error.text);
        return 1;
    }

    FILE *fp = fopen("ID", "rw");
    if (!fp)
    {
        fprintf(stderr, "failed open id\n");
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

    int status;
    if ((status = getaddrinfo(NULL, PORT, &hints, &res)) == -1)
    {
        fprintf(stderr, "getaddrinfo failed: %s\n", gai_strerror(status));
        exit(0);
    };

    if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1)
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
        static char buffer[1024];
        int new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &slen);
        int nCommand = 0;
        set_nonblock(new_fd);
        if (recv(new_fd, &nCommand, sizeof(int), MSG_NOSIGNAL) != sizeof(int))
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

                if (!strcmp(command_text, "create"))
                {
                    json_t *params = json_object_get(object, "params");
                    if (_create(database, params, ID++) == -1)
                        send(new_fd, "error", sizeof("error"), 0);
                    else
                    {
                        json_dump_file(database, BASE_PATH, 0);
                    }
                }
            }
        }
        shutdown(new_fd, SHUT_RDWR);
    }

    return 0;
}