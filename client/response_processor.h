#include <jansson.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>

#include "shared.h"

int response_processor_init();

int proc_response(char *buffer);
