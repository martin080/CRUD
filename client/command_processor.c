#include "command_processor.h"

int command_processor_init()
{
    static int calls = 0;

    if (calls = 0)
        json_decref(templates);
    templates = json_load_file(TEMPLATES_PATH, 0, 0);

    if (!templates)
    {
        fprintf(stderr, "command processor init failed\n");
        return -1;
    }
    calls++;
    return 0;
}

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
    if (check_commands(commands, &errors) == -1)
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

int pack_read_delete(json_t *command, char *buffer, char *command_string)
{
    if (!json_is_object(command))
        return -1;

    json_object_set_new(command, "command", json_string(command_string));

    json_t *params = json_object();
    json_t *messageIDs = json_array();

    int len = strlen(buffer);
    char *cur_num = buffer;

    while (cur_num)
    {
        int n = atoi(cur_num);

        if (n == 0 && *cur_num != 0)
        {
            fprintf(stderr, ">wrong input\n");
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

int create_template(char *path, char *name)
{
    if (!templates)
    {
        fprintf(stderr, ">varibale \"templates\" aren't initialized\n");
        return -1;
    }

    json_error_t error;
    json_t *template = json_load_file(path, 0, &error);
    if (!template)
    {
        fprintf(stderr, ">load template fail: %s\n", error.text);
        return -2;
    }

    struct errors_t errors;
    memset(&errors, 0, sizeof(errors));
    if (check_template(template, &errors) == -1)
    {
        printf_error(&errors);
        json_decref(template);
        return -3;
    }

    json_t *result = json_object();
    json_object_set_new(result, "name", json_string(name));
    json_object_set_new(result, "template", template);

    json_array_append(templates, result);
    return json_dump_file(templates, TEMPLATES_PATH, 0);
}

int template_fill_object(json_t *obj, char *arguments) // при вхождении в рекурсию все ломается из-за указателя (он не перемещается)
{
    char *ptr = arguments;

    while ((*ptr == ' ' || *ptr == '\t') && *ptr != '\0')
        ptr++;

    json_t *value;const char *key;
    json_object_foreach(obj, key, value)
    {
        if (json_is_object(value))
        {
            if (template_fill_object(value, ptr) < 0)
                return -1;
            continue;
        }
        
        if (!json_is_string(value))
            return -1;
        
        const char *content = json_string_value(value);

        if (!strcmp(content, "string"))
        {
            char *string_start, *string_end;
            string_start = strchr(ptr, '"');

            if (!string_start) 
                return -2;
            
            string_end = strchr(string_start + 1, '"');

            *string_end = '\0';

            json_string_set(value, string_start + 1);

            ptr = string_end + 1;
        }
        else if (!strcmp(content, "number"))
        {
            int num = atoi(ptr); // пока что только int

            if (num == 0 && *ptr != '0')
                return -3;
            
            json_t *number_value = json_integer(num);
            json_decref(value);
            value = number_value;

            ptr = strchr(ptr + 1, ' ');
        }
        else
        {
            return -4;
        }
        
    }

    return 0;

}

int template_fill(json_t *obj, char *name, char *argumets)
{
    obj = NULL;
    json_t *value;
    size_t index;
    json_array_foreach(templates, index, value)
    {
        json_t *template_name = json_object_get(value, "name");
        if (!json_is_string(template_name))
            return -1;

        const char *t_ptr = json_string_value(template_name);

        if (!strcmp(t_ptr, name))
        {
            obj = json_object_get(value, "template");
            if (obj == NULL)
                return -1;
            break;
        }
    }

    if (obj == NULL)
        return -1;

    return template_fill_object(obj, argumets);
}

int process_command(int sockfd)
{
    static char buf_command[64], buf_argument[128];

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
    else if (!strcmp(buf_command, "read") || !strcmp(buf_command, "delete"))
    {
        gets(buf_argument, 128);
        char *ptr = buf_argument;

        while ((*ptr == ' ' || *ptr == '\t') && *ptr != '\0')
            ptr++;

        json_t *cmd = json_object();
        int ret = pack_read_delete(cmd, ptr, buf_command);

        if (ret < 0)
            return ret;

        ret = send_command(sockfd, cmd);

        json_decref(cmd);

        return ret;
    }
    else if (!strcmp(buf_command, "create_template"))
    {
        gets(buf_argument, 127);
        char path[32], name[32];
        sscanf(buf_argument, "%s %s", path, name);

        int ret = create_template(path, name);

        if (ret < 0)
            return -1;

        printf(">template was created");
    }
    else if (strstr(buf_command, "create_"))
    {
        char name[32];
        sscanf(buf_command, "create_%s", name);
        gets(buf_argument, 127);

        json_t *request;

        if (template_fill(request, name, buf_argument) < 0)
        {
            fprintf(stderr, ">fill of template failed\n");
            return -1; 
        }

        if (send_command(sockfd, request) < 0)
        {
            fprintf(stderr, ">send failed\n");
            return -1;
        }

        command_processor_init();
    }
    else
    {
        printf(">undefined command");
        fflush(stdin);
        return -1;
    }
}