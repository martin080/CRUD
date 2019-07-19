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
#include "request_handler.h"

#define PORT "3490"
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

int main()
{
    fprintf(stdout, "loading database ...\n");

    int init_res = init_database();
    if (init_res == -1)
    {
        fprintf(stderr, "data base load failed\n");
        return 1;
    }
    else if (init_res == -2)
    {
        fprintf(stderr, "ID init failed\n");
        return 2;
    }
    else
        printf("data base init success\n");

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
            while (new_fd != -1) // new connections
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
            if (pfd[i].revents & POLLHUP) // shut connection  
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

            if (size == 0 && errno != EAGAIN) //shut connection
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

            buffer[size] = '\0';

            printf("  client request: %s \n", buffer);

            json_t *response_object = json_object();

            printf("  processing request ... \n");
            handle_request(buffer, response_object); // handle the request

            char *response_in_text = json_dumps(response_object, JSON_COMPACT);
            printf("  server reponse: %s\n", response_in_text);

            if (send(new_fd, response, strlen(response), 0) == -1)
                printf("send() on socket %d failed\n", new_fd);
            free(response_in_text);

            json_decref(response_object);

            printf("  request was processed \n");
        }
    }
    return 0;
}