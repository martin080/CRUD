#include "data_base.h"

#define data_base_path "data.json"

int load_database(const char *file_path, json_t *object) //doesn't work
{
    FILE *fp = fopen(file_path, "r");
    if (!fp)
    {
        fprintf(stderr, "unable open the file %s\n", file_path);
        return -1;
    }

    json_error_t error;
    object = json_loadf(fp, 0, &error);
    if (!object)
    {
        fprintf(stderr, "error: on line %d: %s\n", error.line, error.text);
        return -1;
    }
}

int create(json_t *data_array, json_t *message, int messageID)
{

    time_t curTime = time(NULL);
    char *time = ctime(&curTime);
    if (json_object_set_new(message, "time", json_stringn(time, strlen(time) - 1)) == -1)
        return -1;

    if (json_object_set_new(message, "messageID", json_integer(messageID)) == -1)
        return -1;

    return json_array_append(data_array, message);
}

int update(json_t *data_array, json_t *message, int messageID)
{
    if (!json_is_array(data_array))
        return -1;

    if (!json_is_object(message))
        return -1;

    json_t *value;
    size_t index;
    json_array_foreach(data_array, index, value)
    {
        json_t *messageID_ = json_object_get(value, "messageID");
        if (!messageID_)
            return -1;
        int msgID = json_integer_value(messageID_);

        if (messageID == msgID)
        {
            return json_object_update(value, message);
        }
    }
    return -1;
}

int delete_object(json_t *data_array, int messageID)
{
    if (messageID < 1)
        return -1;

    if (!json_is_array(data_array))
        return -1;

    size_t index;
    json_t *value;
    json_array_foreach(data_array, index, value)
    {
        json_t *messageID_ = json_object_get(value, "messageID");
        if (!messageID_)
            return -1;
        int msgID = json_integer_value(messageID_);

        if (messageID == msgID)
        {
            json_array_remove(data_array, index);
            return 0;
        }
    }
    return -1;
}

int read_object(json_t *data_array, int messageID, char *buffer, size_t buffer_size)
{
    if (messageID == 0)
    {
        char *object_in_text = json_dumps(data_array, JSON_COMPACT);
        strncpy(buffer, object_in_text, buffer_size - 1);
        free(object_in_text);
        return (strlen(object_in_text) < buffer_size ? 0 : 1);
    }

    if (!json_is_array(data_array))
        return -1;

    size_t index;
    json_t *value;
    json_array_foreach(data_array, index, value)
    {
        json_t *messageID_ = json_object_get(value, "messageID");
        if (!messageID_)
            return -1;
        int msgID = json_integer_value(messageID_);

        if (msgID == messageID)
        {
            char *object_in_text = json_dumps(value, JSON_COMPACT);

            strncpy(buffer, object_in_text, buffer_size - 1);
            free(object_in_text);
            return (strlen(object_in_text) < buffer_size ? 0 : 1);
        }
    }
    return -1;
}