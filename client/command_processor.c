#include "command_processor.h"

char *str_tok(char *buffer)
{

    static int size = 0;
    static char *ptr = NULL;
    static char *buf;

    if (buffer == NULL)
    {
        if (ptr == NULL)
            return NULL;
        while (*ptr != '\0')
        {
            if (ptr >= buf + size)
                return NULL;
            ptr++;
        }

        while (*ptr == '\0')
        {
            if (ptr > buf + size)
                return NULL;
            ptr++;
        }

        if (ptr - buf > size)
            return NULL;

        return ptr;
    }

    size = strlen(buffer);
    buf = buffer;
    int i = 0;
    for (; i < size; i++)
    {
        buffer[i] = (buffer[i] == ' ' || buffer[i] == '\t' || buffer[i] == '\n' ? '\0' : buffer[i]);

        if (buffer[i] == '"')
        {
            buffer[i] = '\0';
            i++;
            while (buffer[i] != '"')
                if (buffer[i++] == '\0')
                {
                    return NULL;
                }
            buffer[i] = '\0';
        }
    }

    ptr = buffer;
    while (*ptr == '\0')
    {
        if (ptr >= buffer + size)
            return NULL;
        ptr++;
    }

    return ptr;
}

int only_numbers(const char *str)
{
    char *ptr = str;

    for ( ; *ptr; ptr++)
        if (*ptr < '0' || *ptr > '9')
            return 0;

    return 1;
}

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

    if (check_commands(commands) == -1)
    {
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

    return 0;
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
        {
            fd = open(file_path, O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
        }
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
        {
            fprintf(stderr, ">changing out path failed\n");
            return -1;
        }

        if (answ == 1)
            write(fd, "[]", 2);

        int flags = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
    else
    {
        write(fd, "[]", 2);
    }

    printf(">out path was changed successully\n");

    if (out_fd != 0)
        close(out_fd);
    out_fd = fd;
    return 0;
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

int handle_option(json_t *obj, char *token)
{
    if (!json_is_object(obj))
        return -1;

    if (token == NULL)
        return -1;

    if (!strcmp(token, "--repo"))
    {
        token = str_tok(NULL);
        if (token == NULL)
            return -1;

        if (strchr(token, '.'))
            return -1;

        json_object_set_new(obj, "repo", json_string(token));
    }
    else
    {
        return -1;
    }

    return 0;
}

int pack_read_delete(json_t *command, char *token, char *command_string)
{
    if (!json_is_object(command))
        return -1;

    json_object_set_new(command, "command", json_string(command_string));

    json_t *params = json_object();
    json_t *messageIDs = json_array();

    while (token)
    {
        if (it_is_option(token))
        {
            if (handle_option(command, token) < 0)
                return -1;
            ;
            token = str_tok(NULL);
        }

        if (token == NULL)
            break;

        if (!only_numbers(token))
        {
            fprintf(stderr, ">wrong input: \"%s\"\n", token);
            return -2;
        }
        int n = atoi(token);

        json_array_append_new(messageIDs, json_integer(n));

        token = str_tok(NULL);
    }

    json_object_set(params, "messageID", messageIDs);
    json_object_set(command, "params", params);

    return 0;
}

int pack_delete_repo(json_t *request, char *name)
{
    if (!json_is_object(request))
        return -1;

    json_object_set_new(request, "command", json_string("delete_repo"));

    json_t *params = json_object();
    json_object_set_new(params, "name", json_string(name));

    json_object_set(request, "params", params);

    return 0;
}

int create_template(char *path, char *name)
{
    if (!templates)
    {
        fprintf(stderr, ">varibale \"templates\" isn't initialized\n");
        return -1;
    }

    if (!strcmp(name, "template"))
    {
        fprintf(stderr, "name \"template\" is unacceptable\n");
        return -1;
    }

    json_t *value;
    size_t index;
    json_array_foreach(templates, index, value)
    {
        json_t *template_name = json_object_get(value, "name");
        if (!json_is_string(template_name))
        {
            fprintf(stderr, ">unknow error with template\n");
            return -1;
        }

        const char *template_name_string = json_string_value(template_name);

        if (!strcmp(name, template_name_string))
        {
            fprintf(stderr, ">name \"%s\" is already taken\n", name);
            return -1;
        }
    }

    json_error_t error;
    json_t *template = json_load_file(path, 0, &error);
    if (!template)
    {
        fprintf(stderr, ">load template fail: %s\n", error.text);
        return -2;
    }

    
    if (check_template(template) == -1)
    {
        json_decref(template);
        return -3;
    }

    json_t *result = json_object();
    json_object_set_new(result, "name", json_string(name));
    json_object_set_new(result, "template", template);

    json_array_append(templates, result);
    return json_dump_file(templates, TEMPLATES_PATH, 0);
}

int template_fill_object(json_t *obj, char *arguments)  // argumtents - string splitted on tokens
{
    if (!json_is_object(obj))
        return -5;

    if (arguments == NULL)
        return -1;

    char *ptr = arguments;

    if (ptr == NULL)
        return -1;

    json_t *value;
    const char *key;
    json_object_foreach(obj, key, value)
    {
        if (ptr == NULL)
            return -1;

        if (json_is_object(value))
        {
            if (template_fill_object(value, ptr) < 0)
                return -1;
            continue;
        }

        if (!json_is_string(value) && !json_is_integer(value))
            return -1;

        const char *content = json_string_value(value);

        if (json_is_string(value))
        {
            const char *val = json_string_value(value);
            if (strcmp(val, "")) // field if filled
                continue;
            json_string_set(value, ptr);
            ptr = str_tok(NULL);
        }
        else if (json_is_integer(value))
        {
            int val = json_integer_value(value);
            if (val != 0) // it means field is filled
                continue;

            int num = atoi(ptr); // пока что только int

            if (num == 0 && *ptr != '0')
                return -3;

            json_integer_set(value, num);
            ptr = str_tok(NULL);
        }
        else
        {
            return -4;
        }
    }

    return 0;
}

int template_fill(json_t **obj, char *name, char *argumets) 
{
    *obj = NULL;
    json_t *value;
    size_t index;
    int is_found = 0;
    json_array_foreach(templates, index, value)
    {
        json_t *template_name = json_object_get(value, "name");
        if (!json_is_string(template_name))
        {
            fprintf(stderr, ">undefined error\n");
            return -1;
        }

        const char *t_ptr = json_string_value(template_name);

        if (!strcmp(t_ptr, name))
        {
            *obj = json_object_get(value, "template");
            if (*obj == NULL)
            {
                fprintf(stderr, ">undefined error\n");
                return -1;
            }
            is_found = 1;
            break;
        }
    }

    if (!is_found)
    {
        printf(">template with name \"%s\" not found\n", name);
        return -1;
    }

    if (argumets == NULL)
    {
        fprintf(stderr, ">tokenization failed\n");
        return -1;
    }

    if (it_is_option(argumets))
    {
        if (handle_option(*obj, argumets) < 0)
        {
            fprintf(stderr, ">handle option failed\n");
            return -1;
        }
        argumets = str_tok(NULL);
    }

    json_t *params = json_object_get(*obj, "params");

    int ret = template_fill_object(params, argumets);

    return ret;
}

int show_template(char *name)
{
    json_t *value;
    size_t index;

    short int all = 0;
    if (!strcmp(name, "--all"))
        all = 1;

    json_array_foreach(templates, index, value)
    {
        json_t *template_name = json_object_get(value, "name");
        if (!json_is_string(template_name))
        {
            fprintf(stderr, ">undefined error\n");
            return -1;
        }

        const char *t_ptr = json_string_value(template_name);

        if (all || !strcmp(t_ptr, name))
        {
            json_t *obj = json_object_get(value, "template");
            if (obj == NULL)
            {
                fprintf(stderr, ">undefined error\n");
                return -1;
            }

            char *template_text = json_dumps(obj, JSON_INDENT(1));
            printf("template \"%s\":\n%s\n", t_ptr, template_text);
            free(template_text);

            if (!all)
                return 0;
        }
    }

    if (!all)
    {
        printf(">template with name \"%s\" not found\n", name);
        return 0;
    }

    return -1;
}

int process_command(int sockfd)
{
    static char command[512];
    char *buf_command, *buf_argument;

    fgets(command, 512, stdin);
    fflush(stdin);

    buf_command = str_tok(command);

    if (!buf_command)
    {
        fprintf(stderr, ">enter the command\n");
        return -1;
    }

    if (!strcmp(buf_command, "exit"))
    {
        close(sockfd);
        return -3;
    }

    buf_argument = str_tok(NULL);

    if (!strcmp(buf_command, "send"))
    {
        if (!buf_argument)
        {
            fprintf(stderr, ">missing arguments: COMMANDS_FILE_PATH\n");
            return -1;
        }
        return send_request(sockfd, buf_argument);
    }
    else if (!strcmp(buf_command, "changeout"))
    {
        if (!buf_argument)
        {
            fprintf(stderr, ">missing argumets: OUT_FILE_PATH\n");
            return -1;
        }
        return changeoutf(buf_argument);
    }
    else if (!strcmp(buf_command, "dump"))
    {
        if (!buf_argument)
        {
            fprintf(stderr, ">missing argumets: OUT_FILE_PATH\n");
            return -1;
        }

        return dump(buf_argument);
    }
    else if (!strcmp(buf_command, "read") || !strcmp(buf_command, "delete"))
    {
        if (!buf_argument)
        {
            fprintf(stderr, ">missing argumets: ID\n");
            return -1;
        }
        char *ptr = buf_argument;

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
        char *path, *name;
        if (buf_argument)
        {
            path = buf_argument;
            buf_argument = str_tok(NULL);

            if (buf_argument)
                name = buf_argument;
            else
            {
                fprintf(stderr, ">wrong usage: create_template PATH TEMPLATE_NAME\n");
                return -1;
            }
        }
        else
        {
            fprintf(stderr, ">wrong usage: create_template PATH TEMPLATE_NAME\n");
            return -1;
        }

        int ret = create_template(path, name);

        if (ret < 0)
            return -1;

        printf(">template was created\n");
    }
    else if (strstr(buf_command, "use_"))
    {
        char name[32];
        sscanf(buf_command, "use_%s", name);

        if (!buf_argument)
        {
            fprintf(stderr, ">missing argumets: ARGS\n");
            return -1;
        }

        char *ptr = buf_argument;

        json_t *request;

        if (template_fill(&request, name, ptr) < 0)
        {
            command_processor_init();
            return -1;
        }

        if (send_command(sockfd, request) < 0)
        {
            fprintf(stderr, ">send failed\n");
            command_processor_init();
            return -1;
        }

        command_processor_init();
    }
    else if (!strcmp(buf_command, "delete_repo"))
    {
        if (!buf_argument)
        {
            fprintf(stderr, ">missing argumets: REPO_NAME");
            return -1;
        }

        json_t *request = json_object();
        if (pack_delete_repo(request, buf_argument) < 0)
            return -1;

        if (send_command(sockfd, request) < 0)
        {
            fprintf(stderr, ">send() failed\n");
            return -1;
        }
    }
    else if (!strcmp(buf_command, "show_template"))
    {
        if (!buf_argument)
        {
            fprintf(stderr, ">missing argumets: TEMPLATE_NAME\n");
            return -1;
        }

        return show_template(buf_argument);
    }
    else
    {
        printf(">undefined command");
        return -1;
    }
}