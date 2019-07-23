#include "server.h"

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

int shut_connection(struct pollfd *pfd, int i, int cur_connections)
{
    pfd[i].revents = 0;
    close(pfd[i].fd);
    for (int j = i; j < cur_connections; j++)
    {
        pfd[j].fd = pfd[j + 1].fd;
        pfd[j].events = pfd[j + 1].events;
        pfd[j].revents = pfd[j + 1].revents;
    }
}

int init_server(char *Port)
{
    int sockfd;
    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int status = getaddrinfo(NULL, Port, &hints, &res);
    if (status == -1)
    {
        fprintf(stderr, "getaddrinfo failed: %s\n", gai_strerror(status));
        return -1;
    };

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1)
    {
        fprintf(stderr, "socket failed.\n");
        return -2;
    }
    set_nonblock(sockfd);

    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1)
    {
        fprintf(stderr, "bind failed: %s\n", strerror(errno));
        return -3;
    }

    if (listen(sockfd, SOMAXCONN) == -1)
    {
        fprintf(stderr, "listen failed.\n");
        return -4;
    }

    freeaddrinfo(res);

    return sockfd;
}

int main()
{
    fprintf(stdout, "  loading database ...\n");

    int init_res = init_database();

    if (init_res == -1)
    {
        fprintf(stderr, "data base load failed\n");
        return 1;
    }
    else if (init_res == -2)
    {
        fprintf(stderr, "ID initialization failed\n");
        return 2;
    }
    else
        printf("  data base initialization success\n");

    int sockfd = init_server(PORT); // initialization of server

    if (sockfd < 0)
        return 1;

    struct pollfd pfd[MAX_CONNECTIONS + 1]; //pollfd initialization
    pfd[0].fd = sockfd;
    pfd[0].events = POLLIN;
    for (int i = 1; i < MAX_CONNECTIONS + 1; i++)
        pfd[i].fd = -1;
    int cur_connections = 0;

    while (1)
    {
        static char buffer[BUFFER_SIZE], buffer_response[BUFFER_SIZE], response[128];
        int ret = poll(pfd, cur_connections + 1, -1);

        if (ret == -1) // poll failed
        {
            fprintf(stderr, "poll failed\n");
            continue;
        }

        if (pfd[0].revents & POLLIN) // it means there are new connections
        {
            int new_fd = 0;
            while (new_fd != -1) // new connections accepting
            {
                new_fd = accept(sockfd, NULL, NULL);
                if (new_fd < 0)
                    continue;

                if (cur_connections < MAX_CONNECTIONS) // if it's possible to connect
                {
                    set_nonblock(new_fd);

                    pfd[cur_connections + 1].fd = new_fd;
                    pfd[cur_connections + 1].events = POLLIN | POLLHUP;
                    cur_connections++;

                    printf("  new connection on socket %d\n", new_fd);

                    continue;
                }

                pack_refuse(buffer, BUFFER_SIZE);
                send(new_fd, buffer, strlen(buffer), 0);

                close(new_fd);
            }
        }
        for (int i = 1; i < cur_connections + 1; i++) //check other connections
        {
            if (pfd[i].revents & POLLHUP) // shut connection
            {
                printf("  connection was lost, socket %d\n", pfd[i].fd);
                shut_connection(pfd, i, cur_connections);
                cur_connections--;
                continue;
            }

            if (!(pfd[i].revents & POLLIN)) // if there is no event - continue
                continue;

            int new_fd = pfd[i].fd;
            pfd[i].revents = 0;

            printf("  reading from socket %d ... \n", new_fd);

            int size = recv(new_fd, buffer, sizeof(buffer), MSG_NOSIGNAL);
            if (size == -1) // error recv()
            {
                printf("recv() on socket %d failed\n", new_fd);
                continue;
            }

            if (size == 0) //shut connection
            {
                printf("  connection was lost, socket %d\n", pfd[i].fd);
                shut_connection(pfd, i, cur_connections);
                cur_connections--;
                continue;
            }   

            char *separator = strstr(buffer, "\n\n"), *tmp_ptr = buffer; // read data until separator

            while (separator)
            {
                *separator = '\0';

                printf("  client request: %s \n", tmp_ptr);

                printf("  processing the request ... \n");

                handle_request(tmp_ptr, buffer_response, BUFFER_SIZE); // handle the request
                printf("  server reponse: %s\n", buffer_response);

                if (send(new_fd, buffer_response, strlen(buffer_response), 0) == -1)
                    printf("send() on socket %d failed\n", new_fd);
                printf("  request was processed \n");

                tmp_ptr = separator + 2; // to the next request
                separator = strstr(tmp_ptr, "\n\n");
            }
        }
    }
    return 0;
}