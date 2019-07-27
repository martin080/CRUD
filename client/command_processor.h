#include "shared.h"
#include "commands_file_parse.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>


char *gets(char *buffer, int buffer_size);

int command_processor_init();

int send_command(int sockfd, json_t *);

int send_request(int sockfd, char *path);

int changeoutf(char *file_path);

int pack_read_delete (json_t *cmd, char *buffer, char *command_string);

int dump(char *path);

int process_command(int sockfd);

