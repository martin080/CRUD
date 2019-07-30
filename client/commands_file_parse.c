#include "commands_file_parse.h"
#include "getmac.h"

int check_commands(json_t *commands)
{
    if (!json_is_array(commands))
    {
        fprintf(stderr, ">file with commands must be array\n");
        return -1;
    }

    json_t *value;
    size_t index;

    json_array_foreach(commands, index, value)
    {
        if (!json_is_object(value))
        {
            fprintf(stderr, ">command %ld is not object\n", index + 1);
            return -1;
        }

        json_t *command = json_object_get(value, "command");
        if (!command)
        {
            fprintf(stderr, ">field \"command\" is required (%ld)\n", index + 1);
            return -1;
        }

        if (!json_is_string(command))
        {
            fprintf(stderr, ">field \"command\" must be a string (%ld)\n",index);
            return -1;
        }

        json_t *params = json_object_get(value, "params");
        if (!params)
        {
            fprintf(stderr, ">field \"params\" is required\n (%ld)", index + 1);
            return -1;
        }

        if (!json_is_object(params))
        {
            fprintf(stderr, ">value of field \"params\" must be object (%ld)\n", index + 1);
            return -1;
        }
        const char *command_value = json_string_value(command);

        if (!strcmp(command_value, "read") || !strcmp(command_value, "delete"))
        {
            json_t *messageID = json_object_get(params, "messageID");

            if (json_is_array(messageID))
            {
                json_t *val;
                size_t ind;
                json_array_foreach(messageID, ind, val)
                {
                    if (!json_is_integer(val))
                    {
                        fprintf(stderr, ">values of array of fielf \"messageID\" must be integers (%ld)\n", index + 1);
                        return -1;
                    }
                }
            }
            else if (json_is_integer(messageID))
            {
                int ID = json_integer_value(messageID);
                json_object_set_new(params, "messageID", json_array());
                json_t *newMessageID = json_object_get(params, "messageID");
                json_array_append_new(newMessageID, json_integer(ID));
            }
            else if (messageID == NULL)
            {
                fprintf(stderr, ">field \"messageID\" is required (%ld)\n", index + 1);
                return -1;
            }
            else
            {
                fprintf(stderr, ">value of field \"messageID\" can be integer or array of integers (%ld)\n", index + 1);
                return -1;
            }
        }
        else if (!strcmp(command_value, "update"))
        {
            json_t *messageID = json_object_get(params, "messageID");
            if (!json_is_integer(messageID))
            {
               fprintf(stderr, ">value of field \"messageID\" must be integer (%ld)\n", index + 1);
                return -1;
            }
        }
        else if (!strcmp(command_value, "create"))
        {
            unsigned char mac[13];
            mac_eth0(mac);
            json_object_set_new(params, "deviceID", json_string(mac));
        }
        else if (!strcmp(command_value, "create_repo") || !strcmp(command_value, "delete_repo"))
        {
            json_t *name = json_object_get(params, "name");

            if (!name)
            {
                fprintf(stderr, ">field \"name\" is required (%ld)\n", index + 1);
                return -1;
            }

            if (!json_is_string(name))
            {
                fprintf(stderr, ">field \"name\" must be a string (%ld)\n", index + 1);
                return -1;
            }
        }
        else
        {
            fprintf(stderr, ">wrong command (%ld)\n", index + 1);
            return -1;
        }
    }
    return 0;
}

int check_template(json_t *template)
{
    if (!json_is_object(template))
    {
        fprintf(stderr, ">template must be an object\n");
        return -1;
    }

    json_t *command = json_object_get(template, "command");

    if (!command)
    {
        fprintf(stderr, ">field \"command\" is required\n");
        return -1;
    }

    if (!json_is_string(command))
    {
        fprintf(stderr, ">field \"command\" must be a string\n");
        return -1;
    }

    const char *content = json_string_value(command);
    if (strcmp(content, "create") && strcmp(content, "create_repo") && strcmp(content, "update"))
    {
        fprintf(stderr, ">field \"command\" in template can be only \"create\", \"create_repo\", \"update\"\n");
        return -1;
    }

    json_t *params = json_object_get(template, "params");

    if (!json_is_object(params))
    {
        fprintf(stderr, ">value of field \"params\" must be an object");
        return -1;
    }

    return 0;
}

