#include <jansson.h>
#include <time.h>
#include <string.h>

int load_database(const char *, json_t *);

int _create(json_t *, json_t *, int);

int _update(json_t *, json_t *, int);

int _delete(json_t *, int );

char* _read(json_t *, int);
