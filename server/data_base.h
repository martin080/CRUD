#include <jansson.h>
#include <time.h>
#include <string.h>

#define BASE_PATH "data.json"
#define ID_PATH "ID"

json_t *data_array;
int messageID;

int init_database();

int create_object(json_t *object);

int update_object(json_t *object, int ID);

int delete_object(int ID);

int read_object( int ID, char *buffer, size_t buffer_size);

