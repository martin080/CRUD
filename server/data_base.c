#include "data_base.h"

#define data_base_path "server/data.json"

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

int _create(json_t *data_array, json_t *message, int messageID)
{

    time_t curTime = time(NULL);
    char *time = ctime(&curTime);
    if (json_object_set_new(message, "time", json_stringn(time, strlen(time) - 1)) == -1)
        return -1;

    if (json_object_set_new(message, "messageID", json_integer(messageID)) == -1)
        return -1;

    return json_array_append(data_array, message);
}

int _update(json_t *data_array, json_t *message, int messageID)
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

int _delete(json_t *data_array, int messageID)
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

char *_read(json_t *data_array, int messageID) // NmessageIDs - указатель на массив, где первый элемент -
{                                              // количество следующих за ним элеменетов (id сообщений)
    if (messageID == 0)
        return json_dumps(data_array, 0);

    if (!json_is_array(data_array)) // Если json_t* - указатель не на массив, возвращаем NULL
        return NULL;

    size_t index;
    json_t *value;
    json_array_foreach(data_array, index, value)
    {
        json_t *messageID_ = json_object_get(value, "messageID");
        if (!messageID_)
            return NULL;
        int msgID = json_integer_value(messageID_);

        if (msgID == messageID)
            return json_dumps(value, 0);
    }
    return NULL;
}