#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/poll.h>
#include <fcntl.h>

#include <sys/ioctl.h>
#include <net/if.h>

#include <jansson.h>
#include <string.h>

#include <unistd.h>
#include <errno.h>

#include <stdio.h>

#include "response_processor.h"
#include "command_processor.h"

#define IP "127.0.0.1"
#define PORT "3490"
// BUFFER_SIZE defined in command_processor.h

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

int init_client(char *Ip, char *Port)
{
    struct addrinfo hints, *res;
    int sockfd;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(Ip, Port, &hints, &res);
    if (status == -1)
    {
        fprintf(stderr, ">getaddrinfo failed: %s\n", gai_strerror(status));
        return -1;
    }

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1)
    {
        fprintf(stderr, ">socket: failed\n");
        return -2;
    };

    printf(">connection to the server ...\n");
    if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1)
    {
        fprintf(stderr, ">connection: failed\n");
        return -3;
    }
    printf(">connect: success\n");

    set_nonblock(sockfd);
    freeaddrinfo(res);

    return sockfd;
}

int main(int argc, char *argv[])
{
    int sockfd = init_client(IP, PORT); // client initialization

    if (sockfd < 0)
        return 1;

    int ret = response_processor_init(); // response processor initialization

    if (ret < 0)
        return 2;

    ret = command_processor_init();

    if (ret < 0)
        return 3;

    struct pollfd pfd[2];
    pfd[0].fd = 0;
    pfd[0].events = POLLIN;

    pfd[1].fd = sockfd;
    pfd[1].events = POLLIN | POLLHUP;

    char buffer[BUFFER_SIZE];
    char buf_command[64], buf_argument[64];

    while (1)
    {
        int ret = poll(pfd, 2, -1);
        if (ret == -1) // poll() failed
        {
            fprintf(stderr, ">poll() failed\n");
            return 0;
        }

        if (pfd[0].revents & POLLIN) // if user wrote smth
        {
            pfd[0].revents = 0;
            int ret = process_command(sockfd);
            
            if (ret == -3)
                return -1;
        }

        if (pfd[1].revents & POLLHUP) // shut connection
        {
            printf(">connection lost\n");
            close(sockfd);
            return 6;
        }

        if (pfd[1].revents & POLLIN) // server sent data
        {
            pfd[1].revents = 0;

            ssize_t size = recv(pfd[1].fd, buffer, BUFFER_SIZE - 1, 0);

            if (size < 0)
            {
                fprintf(stderr, ">recv() failed\n");
                return 8;
            }
            else if (size == 0 && errno != EAGAIN)
            {
                fprintf(stderr, ">connection lost\n");
                close(sockfd);
                return 9;
            }

            buffer[size] = 0;
            printf("> response was recieved \n");
            int status = proc_response(buffer);
        }
    }
}
