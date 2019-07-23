#include "command_processor.h"

int send_request(int sockfd, char *path)
{
    static char buffer[BUFFER_SIZE];
    json_error_t error;
    json_t *commands = json_load_file(path, 0, &error);

    if (!commands)
    {
        printf(">failed load json file: %s\n", error.text);
        return -1;
    }

    struct errors_t errors;
    memset(&errors, 0, sizeof(errors));
    if (check(commands, &errors) == -1)
    {
        printf_error(&errors);
        json_decref(commands);
        return -2;
    }

    json_t *value;
    size_t index;
    json_array_foreach(commands, index, value)
    {
        char *object_in_text = json_dumps(value, JSON_COMPACT);
        strncpy(buffer, object_in_text, BUFFER_SIZE - 3);
        free(object_in_text);

        int size = strlen(buffer);
        strcpy(buffer + size, "\n\n");

        if (send(sockfd, buffer, strlen(buffer), 0) == -1)
            fprintf(stderr, ">send() failed\n");
    }
}

int process_command(int sockfd)
{
    static char buf_command[64], buf_argument[64];

    scanf("%s", buf_command);

    if (!strcmp(buf_command, "send"))
    {
        scanf("%s", buf_argument);

        send_request(sockfd, buf_argument);
    }
    else if (!strcmp(buf_command, "changeout"))
    {
        scanf("%s", buf_argument);
        return changeoutf(buf_argument);
    }
    else if (!strcmp(buf_command, "dump"))
    {
        scanf("%s", buf_argument);
        
        json_error_t error;
        json_t *new_array = json_load_file(buf_argument, 0, &error);
        if (!json_is_array(new_array))
            new_array = json_array();
        
        json_array_extend(out_array, new_array);

        return json_dump_file(out_array, buf_argument, 0);
    }
    else if (!strcmp(buf_command, "exit"))
    {
        close(sockfd);
        return -3;
    }
    else
        printf("undefined command");
}