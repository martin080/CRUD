#include <jansson.h>
#include <time.h>
#include <string.h>

int load_database(const char *, json_t *);

int create(json_t *, json_t *, int);

int update(json_t *, json_t *, int);

int delete_object(json_t *, int );

char* read(json_t *, int, const char *, size_t);
