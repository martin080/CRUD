#include "response_processor.h"
#include "commands_file_parse.h"
#include <sys/types.h>
#include <sys/socket.h>

#define BUFFER_SIZE 4096

int send_request(int sockfd, char *path);

int process_command(int sockfd);
