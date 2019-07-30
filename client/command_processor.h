#include "shared.h"
#include "commands_file_parse.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>

int command_processor_init();

int send_command(int sockfd, json_t *);

int send_request(int sockfd, char *path);

int changeoutf(char *file_path);

int dump(char *path);

int handle_option(json_t *request, char *opt);

int pack_read_delete (json_t *cmd, char *buffer, char *command_string);

int pack_delete_repo(json_t *request, char *name);

int create_template(char *path, char *name);

int template_fill_object(json_t *obj, char* args);

int template_fill(json_t **obj, char *name, char *args);

int show_template(char *name);

int process_command(int sockfd);

