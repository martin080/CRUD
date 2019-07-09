#include <jansson.h>
#include <string.h>

struct errors_t
{
    short int is_errors;
    short int command_index;
    short int is_not_array;
    short int is_not_object;
    short int there_is_no_command;
    short int there_is_no_params;
    short int command_is_not_string;
    short int wrong_command;
    short int there_is_no_deviceid;
    short int messageid_is_not_integer;
    short int deviceid_is_not_a_string;
};


int check(json_t *, struct errors_t *);

void printf_error(struct errors_t *);