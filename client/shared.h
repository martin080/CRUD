#include <jansson.h>

#define BUFFER_SIZE 4096
#define TEMPLATES_PATH "templates.json"

json_t *templates;
int out_fd;
int last_recieved;
json_t *out_array;