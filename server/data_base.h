#include <jansson.h>
#include <string.h>
#include <time.h>
#include <sys/types.h> // open() \/
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h> // write()

#define BASE_PATH "storage/data.json"
#define STORAGE_FOLDER "storage"
#define STD_BASENAME_WITHOUT_EXTENTION "data"

json_t *database;
json_t *data_array;
json_t *repo_info;
int messageID;
short int is_repo_default;

int init_database();

int dump_database();

int create_repo(const char *name);

int delete_repo(const char *name);

int change_repo(const char *name);

int create_object(json_t *object);

int update_object(json_t *object, int ID);

int delete_object(int ID);

int read_object( int ID, char *buffer, size_t buffer_size);

