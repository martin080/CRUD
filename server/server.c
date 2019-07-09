#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "data_base.h"

#define PORT "3490"

int main()
{
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

    if ((sockfd = socket(res -> ai_family, res -> ai_socktype, res -> ai_protocol)) == -1)
    {
        fprintf(stderr, "socket failed.\n");
        exit(1);
    }

    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    if (bind(sockfd, res -> ai_addr, res -> ai_addrlen) == -1)
    {
        fprintf(stderr,"bind failed.\n");
        exit(2);
    }

    if (listen(sockfd, SOMAXCONN) == -1)
    {
        fprintf(stderr, "listen failed.\n");
        exit(3);
    }

    freeaddrinfo(res);

    while (1)
    {
        
    }

    return 0;
}