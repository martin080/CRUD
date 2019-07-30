#include <jansson.h>

#define BUFFER_SIZE 4096
#define TEMPLATES_PATH "templates.json"

#define it_is_option(token) (!strstr(token, "--") == 0)

json_t *templates;
int out_fd;
int last_recieved;
json_t *out_array;