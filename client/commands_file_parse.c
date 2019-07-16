#include "commands_file_parse.h"
#include "getmac.h"

int check(json_t *commands, struct errors_t *errors)
{
    if (!json_is_array(commands))
    {
        errors->is_errors = 1;
        errors->is_not_array = 1;
        return -1;
    }

    json_t *value;
    size_t index;

    json_array_foreach(commands, index, value)
    {
        if (!json_is_object(value))
        {
            errors->command_index = index;
            errors->is_errors = 1;
            errors->is_not_object = 1;
            return -1;
        }

        json_t *command = json_object_get(value, "command");
        if (!command)
        {
            errors->is_errors = 1;
            errors->command_index = index;
            errors->there_is_no_command = 1;
            return -1;
        }

        if (!json_is_string(command))
        {
            errors->is_errors = 1;
            errors->command_index = index;
            errors->command_is_not_string = 1;
            return -1;
        }

        json_t *params = json_object_get(value, "params");
        if (!params)
        {
            errors->is_errors = 1;
            errors->command_index = index;
            errors->there_is_no_params = 1;
            return -1;
        }

        if (!json_is_object(params))
        {
            errors->is_errors = 1;
            errors->command_index = index;
            errors->is_not_object = 1;
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
                        errors->is_errors = 1;
                        errors->messageid_is_not_integer = 1;
                        errors->command_index = index;
                        return -1;
                    }
                }
            }
            else if (!json_is_integer(messageID))
            {
                errors->is_errors = 1;
                errors->command_index = index;
                errors->messageid_is_not_integer = 1;
                return -1;
            }
        }
        else if (!strcmp(command_value, "update"))
        {
            json_t *messageID = json_object_get(params, "messageID");
            if (!json_is_integer(messageID))
            {
                errors->is_errors = 1;
                errors->command_index = index;
                errors->messageid_is_not_integer = 1;
                return -1;
            }
        }
        else if (!strcmp(command_value, "create"))
        {
            unsigned char mac[13];
            mac_eth0(mac);
            json_object_set_new(params, "deviceID", json_string(mac));
        }
        else
        {
            errors->is_errors = 1;
            errors->command_index = index;
            errors->wrong_command = 1;
            return -1;
        }
    }

    errors->is_errors = 0;
    return 0;
}

void printf_error(struct errors_t *errors)
{
    if (!errors->is_errors)
    {
        printf("ok\n");
        return;
    }
    else if (errors->is_not_array)
    {
        fprintf(stderr, "commands are not in the array\n");
        return;
    }
    else if (errors->is_not_object)
    {
        fprintf(stderr, "command %d is not object\n", errors->command_index + 1);
        return;
    }
    else if (errors->command_is_not_string)
    {
        fprintf(stderr, "field \"command\" in command %d is not a string\n", errors->command_index + 1);
        return;
    }
    else if (errors->there_is_no_params)
    {
        fprintf(stderr, "in command %d there is no field \"params\"\n", errors->command_index + 1);
        return;
    }
    else if (errors->is_not_object)
    {
        fprintf(stderr, "field \"params\" in command %d is not an object\n", errors->command_index + 1);
        return;
    }
    else if (errors->there_is_no_deviceid)
    {
        fprintf(stderr, "there is no deviceID in command %d\n", errors->command_index + 1);
        return;
    }
    else if (errors->messageid_is_not_integer)
    {
        fprintf(stderr, "in command %d messageID aren't integers\n", errors->command_index + 1);
        return;
    }
    else if (errors->wrong_command)
    {
        fprintf(stderr, "wrong field \"command\" in command %d\n", errors->command_index + 1);
        return;
    }
    else if (errors->deviceid_is_not_a_string)
    {
        fprintf(stderr, "deviceID in command %d is not a string\n", errors->command_index + 1);
        return;
    }

    fprintf(stderr, "undefined error in command %d\n", errors->command_index + 1);
}