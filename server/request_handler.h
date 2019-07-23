#include <jansson.h>
#include <stdio.h>
#include "data_base.h"

#define BUFFER_SIZE 4096

int init_handler();

int pack_status(json_t *response, int status);

int pack_message(json_t *response, char *message);

int handle_create( json_t *request, json_t *response);

int handle_read( json_t *request, json_t *response);

int handle_update( json_t *request, json_t *response);

int handle_delete(json_t *request, json_t *response);

int handle_request(char *request, char *buffer, size_t buffer_size);