#include "response_processor.h"

int init_processor()
{
    out_fd = 0;
    last_recieved = 0;
    out_array = json_array();
}

int changeoutf(char *file_path)
{
    if (!strcmp(file_path, "stdout"))
    {
        if (out_fd != 0)
            close(out_fd);
        out_fd = 0;
        printf(">out path was changed successully\n");

        return 0;
    }

    int flags = O_RDWR | O_EXCL | O_CREAT;
    int fd = open(file_path, flags, S_IRUSR | S_IWUSR);

    if (fd == -1)
    {
        fprintf(stdout, ">File with name \"%s\" already exists, do you want to rewrite or append it?(1/2)\n", file_path);
        int answ;
        scanf("%d", &answ);
        if (answ == 1)
            fd = open(file_path, O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
        else
        {
            fd = open(file_path, O_RDWR);
            json_error_t error;
            json_t *old_out_array = out_array;
            out_array = json_loadfd(fd, 0, &error);
            if (!out_array)
            {
                fprintf(stderr, ">failed load to json: %s\n", error.text);
                out_array = old_out_array;
                json_decref(old_out_array);
                return -2;
            }
        }

        if (fd == -1)
            return -1;
        printf(">out path was changed successully\n");

        int flags = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }

    if (out_fd != 0)
        close(out_fd);
    out_fd = fd;
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
        json_dumpfd(out_array, out_fd, 0);
    }

    return 0;
}