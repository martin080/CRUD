#include "request_handler.h"

int load_id(int ID)
{
    FILE *id_file = fopen("ID", "r+");
    if (!id_file)
    {
        fprintf(stderr, "ID file open failed\n");
        return -1;
    }

    int ret = fprintf(id_file, "%d", ID);

    fclose(id_file);

    return ret;
}

int pack_status(json_t *response, int status)
{
    if (!json_is_object(response))
        return -1;

    json_t *res = json_object_get(response, "status");

    if (!res)
        return json_object_set_new(response, "status", json_integer(status));
    else
        return json_integer_set(res, status);
}

int pack_message(json_t *response, char *message)
{
    if (!json_is_object(response))
        return -1;

    return json_object_set_new(response, "message", json_string(message));
}

int pack_refuse(char *buffer, size_t buffer_size)
{
    int ret = snprintf(buffer, buffer_size, "{\"status\":-1,\"result\":{\"message\":\"server is unable to server you, please try later\"}}\n\n");
    return ret;
}

int handle_create(json_t *request, json_t *response)
{
    if (!json_is_array(data_array))
        return -3;

    if (!json_is_object(response))
        return -2;

    json_object_set_new(response, "operation", json_string("create"));
    json_t *result = json_object();

    static char response_text[128];

    if (create_object(request) == -1)
    {
        snprintf(response_text, 128, "data load failed");
        pack_status(response, -1);
        pack_message(result, response_text);
        json_object_set(response, "result", result);
        return -1;
    }
    else
    {
        snprintf(response_text, 128, "data was loaded successfuly, new messageID is %d", messageID);
        pack_status(response, 0);
        pack_message(result, response_text);
        json_object_set(response, "result", result);
        load_id(++messageID);
    }
    return 0;
}

int handle_read(json_t *params, json_t *response)
{
    if (!json_is_array(data_array))
        return -3;

    if (!json_is_object(response))
        return -2;

    json_object_set_new(response, "operation", json_string("read"));

    json_t *result = json_object();
    pack_status(response, 0);

    static char buffer[BUFFER_SIZE];
    static char response_text[128];

    json_t *ids = json_object_get(params, "messageID");
    if (!json_is_array(ids))
    {
        pack_status(response, -1);
        pack_message(result, "field \"messageID\" is not array");
        json_object_set(response, "result", result);
        return -1;
    }

    json_t *success = json_object();
    json_t *fail = json_array();

    json_t *value;
    size_t index;
    json_array_foreach(ids, index, value)
    {
        if (!json_is_integer(value))
        {
            pack_status(response, -1);
            pack_message(result, "value in field \"messageID\" is not integer");
            json_object_set(result, "success", success);
            json_object_set(result, "fail", fail);
            json_object_set(response, "result", result);
            return -1;
        }

        int ID = json_integer_value(value);
        ssize_t size = read_object(ID, buffer, BUFFER_SIZE - 1);
        if (size < 0)
        {
            pack_status(response, -1);
            json_array_append_new(fail, json_integer(ID));
            continue;
        };

        buffer[size] = '\0';
        char id_text[11];
        snprintf(id_text, 11, "ID:%d", ID);

        json_object_set_new(success, id_text, json_string(buffer));
    }

    json_object_set(result, "succes", success);
    json_object_set(result, "fail", fail);
    json_object_set(response, "result", result);

    return 0;
}

int handle_update(json_t *request, json_t *response)
{
    if (!json_is_array(data_array))
        return -3;

    if (!json_is_object(response))
        return -2;

    json_object_set_new(response, "operation", json_string("update"));
    json_t *result = json_object();

    json_t *messageID = json_object_get(request, "messageID");

    if (!messageID)
    {
        pack_status(response, -1);
        pack_message(result, "lost meessage ID");
        json_object_set(response, "result", result);
        return -1;
    }

    if (!json_is_integer(messageID))
    {
        pack_status(response, -1);
        pack_message(result, "message ID is not interger");
        json_object_set(response, "result", result);
        return -1;
    }

    int ID = json_integer_value(messageID);
    if (update_object(request, ID) == -1)
    {
        pack_status(response, -1);
        pack_message(result, "update is failed");
        json_object_set(response, "result", result);
        return -1;
    }

    static char response_text[128];
    snprintf(response_text, 128, "message with messageID %d was updated succesfuly", ID);
    pack_status(response, 0);
    pack_message(result, response_text);
    json_object_set(response, "result", result);

    return 0;
}

int handle_delete(json_t *request, json_t *response)
{
    if (!json_is_object(response))
        return -2;

    json_object_set_new(response, "operation", json_string("delete"));

    json_t *messageID = json_object_get(request, "messageID");
    if (!messageID)
    {
        pack_status(response, -1);
        pack_message(response, "message ID not found");
        return -1;
    }

    if (!json_is_array(messageID))
    {
        pack_status(response, -1);
        pack_message(response, "field \"messageID\" is not array");
        return -1;
    }

    json_t *result = json_object();

    pack_status(response, 0);

    json_t *success = json_array();
    json_t *fail = json_array();

    json_t *value;
    size_t index;
    json_array_foreach(messageID, index, value)
    {
        if (!json_is_integer(value))
        {
            json_array_append(fail, value);
            pack_status(response, 1);
            continue;
        }

        if (delete_object(json_integer_value(value)) == -1)
        {
            json_array_append(fail, value);
            pack_status(response, 1);
            continue;
        }
        else
        {
            json_array_append(success, value);
            continue;
        }
    }
    json_object_set(result, "success", success);
    json_object_set(result, "fail", fail);
    json_object_set(response, "result", result);

    return 0;
}

int handle_request(char *request, char *buffer, size_t buffer_size)
{
    json_t *response_object = json_object();

    pack_status(response_object, 0);
    json_t *result = json_object();

    json_error_t error;
    json_t *object = json_loads(request, 0, &error);
    if (!object)
    {
        pack_status(response_object, -1);
        pack_message(result, "wrong request format");
        json_object_set(response_object, "result", result);
        return -1;
    }

    json_t *command = json_object_get(object, "command");
    json_t *params = json_object_get(object, "params");
    const char *command_text = json_string_value(command);
    int status;

    if (!strcmp(command_text, "create")) // command == create
        status = handle_create(params, response_object);
    else if (!strcmp(command_text, "read")) //command == read
        status = handle_read(params, response_object);
    else if (!strcmp(command_text, "update")) // command == update
        status = handle_update(params, response_object);
    else if (!strcmp(command_text, "delete")) // command == delete
        status = handle_delete(params, response_object);

    json_decref(command);
    json_decref(params);

    char *response_in_text = json_dumps(response_object, JSON_COMPACT);
    if (response_in_text == NULL) // if json_dumps() failed, fill the response_object
    {
        json_decref(response_object);
        response_object = json_object();
        pack_status(response_object, -1);
        json_t *result = json_object();
        pack_message(result, "failed dump json data\n");
        json_object_set(response_object, "result", result);
        response_in_text = json_dumps(response_object, JSON_COMPACT);
    }
    snprintf(buffer, buffer_size, "%s\n\n", response_in_text);
    free(response_in_text);
    json_decref(response_object);

    return status;
}