#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <jansson.h>
#include <string.h>

#include <stdio.h>
#include "commands_file_parse.c"

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "usage: ./client COMMAND_FILE\n");
        return 1;
    }

    json_error_t error;
    json_t *commands = json_load_file(argv[1], 0, &error);
    if (!commands)
    {
        fprintf(stderr, "failed load the file: %s\n", error.text);
        return 2;
    }

    struct errors_t errors;
    memset(&errors, 0, sizeof(errors));
    if (check(commands, &errors) == -1)
    {
        printf_error(&errors);
        return 3;
    }

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

    if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1)
    {
        fprintf(stderr, "connection: failed");
        return 6;
    }

    int n = json_array_size(commands);
    send(sockfd, &n, sizeof(n), 0);

    json_t *value; size_t index;
    json_array_foreach(commands, index, value)
    {
        char *json_in_text = json_dumps(value, JSON_COMPACT);
        send(sockfd, json_in_text, strlen(json_in_text), 0);
        free(json_in_text);
    }

    static char buffer[1024]; int size;
    while ((size = recv(sockfd, buffer, sizeof(buffer), MSG_NOSIGNAL)))
    {
        buffer[size++] = '\0';
        printf("%s\n", buffer);
    }

    freeaddrinfo(res);
}