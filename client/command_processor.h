#include "response_processor.h"
#include "commands_file_parse.h"
#include <sys/types.h>
#include <sys/socket.h>

#define BUFFER_SIZE 4096

char *gets(char *buffer, int buffer_size);

int send_command(int sockfd, json_t *);

int send_request(int sockfd, char *path);

int pack_read(json_t *cmd, char *buffer);

int dump(char *path);

int process_command(int sockfd);

