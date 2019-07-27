#include "command_processor.h"

char *gets(char *s, int buffer_size)
{
    fflush(stdin);
    int i, k = getchar();
    if (k == EOF)
        return NULL;
    for (i = 0; k != EOF && k != '\n' && i < buffer_size; ++i)
    {
        s[i] = k;
        k = getchar();
        if (k == EOF && !feof(stdin))
            return NULL;
    }
    s[i] = '\0';

    return s;
}

int send_command(int sockfd, json_t *cmd)
{
    if (!cmd)
        return -1;

    if (!json_is_object(cmd))
        return -2;

    char buffer[BUFFER_SIZE];

    char *object_in_text = json_dumps(cmd, JSON_COMPACT);
    strncpy(buffer, object_in_text, BUFFER_SIZE - 3);
    free(object_in_text);

    int size = strlen(buffer);
    strcpy(buffer + size, "\n\n");

    return send(sockfd, buffer, strlen(buffer), 0);
}

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
        if (send_command(sockfd, value) == -1)
            fprintf(stderr, ">send() failed\n");
    }
}

int dump(char *path)
{
    json_error_t error;
    json_t *new_array = json_load_file(path, 0, &error);
    if (!json_is_array(new_array))
        new_array = json_array();

    json_array_extend(out_array, new_array);

    int ret = json_dump_file(out_array, path, 0);
    if (ret == 0)
        printf(">data was dumped successfully\n");
    else
        fprintf(stderr, ">data dump failed\n");

    return ret;
}

int pack_read(json_t *command, char *buffer)
{
    if (!json_is_object(command))
        return -1;

    json_object_set_new(command, "command", json_string("read"));

    json_t *params = json_object();
    json_t *messageIDs = json_array();

    int len = strlen(buffer);
    char *cur_num = buffer;

    while (cur_num)
    {
        int n = atoi(cur_num);

        if (n == 0 && *cur_num != 0)
        {
            fprintf(stderr, "wrong input\n");
            return -2;
        }

        json_array_append_new(messageIDs, json_integer(n));

        cur_num = strchr(cur_num, ' ');

        if (cur_num)
            cur_num++;
    }

    json_object_set(params, "messageID", messageIDs);
    json_object_set(command, "params", params);

    return 0;
}

int process_command(int sockfd)
{
    static char buf_command[64], buf_argument[64];

    scanf("%s", buf_command);

    if (!strcmp(buf_command, "send"))
    {
        scanf("%s", buf_argument);
        fflush(stdin);
        return send_request(sockfd, buf_argument);
    }
    else if (!strcmp(buf_command, "changeout"))
    {
        scanf("%s", buf_argument);
        fflush(stdin);
        return changeoutf(buf_argument);
    }
    else if (!strcmp(buf_command, "dump"))
    {
        scanf("%s", buf_argument);
        fflush(stdin);

        return dump(buf_argument);
    }
    else if (!strcmp(buf_command, "exit"))
    {
        close(sockfd);
        fflush(stdin);
        return -3;
    }
    else if (!strcmp(buf_command, "read"))
    {
        gets(buf_argument, 64);
        char *ptr = buf_argument;

        while ((*ptr == ' ' || *ptr == '\t') && *ptr != '\0')
            ptr++;

        json_t *cmd = json_object();
        int ret = pack_read(cmd, ptr);

        if (ret < 0)
            return ret;

        ret = send_command(sockfd, cmd);

        json_decref(cmd);

        return ret;
    }
    else
    {
        printf("undefined command");
        fflush(stdin);
        return -1;
    }
}