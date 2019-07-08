#include <jansson.h>
#include <time.h>
#include <string.h>

int load_database(const char *, json_t *);

int create(json_t *, const char *, const char *, int);

int update(json_t *, const char *, int);

int _delete(json_t *, int *);

char* read(json_t *, int *);