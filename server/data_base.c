#include "data_base.h"

int init_database()
{
    json_error_t error;
    data_array = json_load_file(BASE_PATH, 0, &error);
    if (!data_array)
        return -1;

    FILE *ID = fopen(ID_PATH, "r+");

    if (ID == NULL)
        return -2;

    fscanf(ID, "%d", &messageID);

    return 0;
}

int create_object(json_t *message)
{
    if (!json_is_array(data_array))
        return -2;

    if (messageID == 0)
        return -3;

    time_t curTime = time(NULL);
    char *time = ctime(&curTime);
    if (json_object_set_new(message, "time", json_stringn(time, strlen(time) - 1)) == -1)
        return -1;

    if (json_object_set_new(message, "messageID", json_integer(messageID)) == -1)
        return -1;

    int ret = json_array_append(data_array, message);
    json_dump_file(data_array, BASE_PATH, 0);

    return ret;
}

int update_object(json_t *message, int messageID)
{
    if (!json_is_array(data_array))
        return -2;

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
            time_t curTime = time(NULL);
            char *time = ctime(&curTime);
            if (json_object_set_new(value, "time", json_stringn(time, strlen(time) - 1)) == -1)
                return -1;
            int ret = json_object_update(value, message);
            json_dump_file(data_array, BASE_PATH, 0);
            return ret;
        }
    }
    return -1;
}

int delete_object(int messageID)
{
    if (!json_is_array(data_array))
        return -2;

    if (messageID < 1)
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
            json_dump_file(data_array, BASE_PATH, 0);
            return 0;
        }
    }
    return -1;
}

int read_object(int search_ID, char *buffer, size_t buffer_size)
{
    if (!json_is_array(data_array))
        return -2;

    if (search_ID > messageID || search_ID < 0)
        return -3;

    if (search_ID == 0)
    {
        char *object_in_text = json_dumps(data_array, JSON_COMPACT);
        if (!object_in_text)
            return -1;
        strncpy(buffer, object_in_text, buffer_size - 1);
        int len = strlen(object_in_text);
        free(object_in_text);
        return (len < buffer_size ? len : buffer_size);
    }

    if (!json_is_array(data_array))
        return -1;

    size_t index;
    json_t *value;
    json_array_foreach(data_array, index, value)
    {
        json_t *messageID_ = json_object_get(value, "messageID");
        if (!messageID_ || !json_is_integer(messageID_))
            continue;

        int msgID = json_integer_value(messageID_);

        if (msgID == search_ID)
        {
            char *object_in_text = json_dumps(value, JSON_COMPACT);
            if (!object_in_text)
                return -1;

            strncpy(buffer, object_in_text, buffer_size - 1);
            int len = strlen(object_in_text);
            free(object_in_text);
            return (len < buffer_size ? len : buffer_size);
        }
    }
    return -1;
}