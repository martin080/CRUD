#include "response_processor.h"

int response_processor_init()
{
    out_fd = 0;
    last_recieved = 0;
    out_array = json_array();
}


int proc_response(char *buffer)
{
    last_recieved = 0;
    char *separator = strstr(buffer, "\n\n"), *tmp_ptr = buffer;

    while (separator)
    {
        *separator = 0;

        json_error_t error;
        json_t *response = json_loads(tmp_ptr, 0, &error);

        if (json_is_object(response))
        {
            json_array_append(out_array, response);
            last_recieved++;
            json_decref(response);
        }
        else
        {
            fprintf(stderr, ">failed load object: %s\n", error.text);
        }

        tmp_ptr = separator + 3;
        separator = strstr(buffer, "\n\n");
    }

    if (out_fd == 0)
    {
        size_t array_size = json_array_size(out_array);
        for (size_t i = array_size - last_recieved; i < array_size; i++)
        {
            json_t *value = json_array_get(out_array, i);
            char *ptr = json_dumps(value, JSON_COMPACT);
            printf("%s\n", ptr);
            free(ptr);
        }
    }
    else
    {
        lseek(out_fd, 0, SEEK_SET);
        int ret = json_dumpfd(out_array, out_fd, 0);
        if (ret == -1)
        {
            fprintf(stderr, ">load datat failed\n");
            return -1;
        }
    }

    return 0;
}