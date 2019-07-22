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
#include "commands_file_parse.h"

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

int main(int argc, char *argv[])
{
    struct addrinfo hints, *res;
    int sockfd;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int status;
    if ((status = getaddrinfo("127.0.0.1", "3490", &hints, &res)) == -1)
    {
        fprintf(stderr, "getaddrinfo failed: %s\n", gai_strerror(status));
        return 4;
    }

    if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1)
    {
        fprintf(stderr, "socket: failed\n");
        return 5;
    };

    printf("connection to the server ...\n");
    if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1)
    {
        fprintf(stderr, "connection: failed\n");
        return 6;
    }
    printf("connect: success\n");

    set_nonblock(sockfd);
    freeaddrinfo(res);

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
        if (ret == -1)
        {
            fprintf(stderr, "poll() failed\n");
            return 0;
        }

        if (pfd[0].revents & POLLIN)
        {
            pfd[0].revents = 0;
            scanf("%s", buf_command);

            if (!strcmp(buf_command, "send"))
            {
                scanf("%s", buf_argument);
                json_error_t error;
                json_t *commands = json_load_file(buf_argument, 0, &error);
                if (!commands)
                {
                    fprintf(stderr, "  failed load the file \"%s\": %s\n", buf_argument, error.text);
                    json_decref(commands);
                    continue;
                }

                struct errors_t errors;
                memset(&errors, 0, sizeof(errors));
                if (check(commands, &errors) == -1)
                {
                    printf_error(&errors);
                    json_decref(commands);
                    continue;
                }

                json_t *value;
                size_t index;
                json_array_foreach(commands, index, value)
                {
                    char *object_in_text = json_dumps(value, JSON_COMPACT);
                    send(sockfd, object_in_text, strlen(object_in_text), 0);
                    free(object_in_text);

                    usleep(1000);
                }
            }
            else if (!strcmp(buf_command, "exit"))
            {
                close(sockfd);
                return 7;
            }
            else
                printf("wrong command \"%s\"\n", buf_command);
        }

        if (pfd[1].revents & POLLHUP)
        {
            printf("  connection lost\n");
            close(sockfd);
            return 6;
        }

        if (pfd[1].revents & POLLIN)
        {
            pfd[1].revents = 0;

            ssize_t size = recv(pfd[1].fd, buffer, BUFFER_SIZE - 1, 0);

            while (size == BUFFER_SIZE - 1)
            {
                buffer[size] = '\0';
                printf("%s", buffer);
                size = recv(pfd[1].fd, buffer, BUFFER_SIZE - 1, 0);
            }

            if (size < 0)
            {
                fprintf(stderr, "recv() failed\n");
                return 8;
            }
            else if (size == 0 && errno != EAGAIN)
            {
                fprintf(stderr, "connection lost\n");
                return 9;
            }

            buffer[size] = '\0';
            printf("%s\n", buffer);
        }
    }
}