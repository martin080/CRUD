#include <jansson.h>
#include <time.h>
#include <string.h>

int load_database(const char *, json_t *);

int _create(json_t *, const char *, const char *, int);

int _update(json_t *, const char *, int);

int _delete(json_t *, int *);

char* _read(json_t *, int *);

