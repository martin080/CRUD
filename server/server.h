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
#define BUFFER_SIZE 4096

int set_nonblock(int fd);

int shut_connection(struct pollfd *pfd, int i, int cur_connections);

int init_server(char *Port);
