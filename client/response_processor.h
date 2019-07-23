#include <jansson.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>

int out_fd;
int last_recieved;
json_t *out_array;

int init_processor();

int changeoutf(char *file_path);

int proc_response(char *buffer);
