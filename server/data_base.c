#include "data_base.h"

#define data_base_path "server/data.json"

void qs(int *s_arr, int first, int last)
{
    if (first < last)
    {
        int left = first, right = last, middle = s_arr[(left + right) / 2];
        do
        {
            while (s_arr[left] < middle)
                left++;
            while (s_arr[right] > middle)
                right--;
            if (left <= right)
            {
                int tmp = s_arr[left];
                s_arr[left] = s_arr[right];
                s_arr[right] = tmp;
                left++;
                right--;
            }
        } while (left <= right);
        qs(s_arr, first, right);
        qs(s_arr, left, last);
    }
}

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

int _create(json_t *data_array, const char *message, const char *deviceID, int messageID)
{
    json_error_t error;
    json_t *object = json_loads(message, 0, &error);
    if (!object)
    {
        fprintf(stderr, "%s", error.text);
        return -1;
    }

    time_t curTime = time(NULL);
    char *time = ctime(&curTime);
    if (json_object_set_new(object, "time", json_stringn(time, strlen(time) - 1)) == -1)
        return -1;

    if (json_object_set_new(object, "messageID", json_integer(messageID)) == -1)
        return -1;

    if (json_object_set_new(object, "deviceID", json_string(deviceID)))
        return -1;

    return json_array_append(data_array, object);
}

int _update(json_t *data_array, const char *message, int messageID)
{
    if (!json_is_array(data_array))
        return -1;

    json_error_t error;
    json_t *object = json_loads(message, 0, &error);

    if (!object)
    {
        fprintf(stderr, "%s\n", error.text);
        return -1;
    }

    json_t *target_object = json_array_get(data_array, messageID - 1);

    return json_object_update(target_object, object);
}

int _delete(json_t *data_array, int *NmessageIDs)
{
    int n = *NmessageIDs;
    if (n < 1)
        return -1;

    if (!json_is_array(data_array))
        return -1;

    qs(NmessageIDs + 1, 0, n - 1);

    int *start = NmessageIDs + 1, *oldStart = start;

    size_t index;
    json_t *value;
    json_array_foreach(data_array, index, value)
    {
        json_t *messageID = json_object_get(value, "messageID");
        if (!messageID)
            return -1;
        int msgID = json_integer_value(messageID);

        int *p = start;
        int i = n;
        while (i > 0)
        {
            if (*p == msgID)
            {
                json_array_remove(data_array, index);
                oldStart = start;
                start = p + 1;
                n = n - (start - oldStart);
                index--;
                break;
            }
            p++;
            i--;
        }
    }
}

char *_read(json_t *data_array, int *NmessageIDs) // NmessageIDs - указатель на массив, где первый элемент -
{                                                // количество следующих за ним элеменетов (id сообщений)
    if (*NmessageIDs == 0)                       // Если количество id == 0, возвращаем все записи базы
        return json_dumps(data_array, 0);

    if (!json_is_array(data_array))             // Если json_t* - указатель не на массив, возвращаем NULL
        return NULL;

    json_t *res_object = json_array();

    int n = *NmessageIDs;
    qs(NmessageIDs + 1, 0, n - 1); //quick sort

    int *start = NmessageIDs + 1, *oldStart = start;
    size_t index;
    json_t *value;
    json_array_foreach(data_array, index, value)     
    {
        json_t *messageID = json_object_get(value, "messageID");
        if (!messageID)
            return NULL;
        int msgID = json_integer_value(messageID);

        int *p = start;
        int i = n;
        while (i > 0)
        {
            if (*p == msgID)
            {
                json_array_append(res_object, value);
                oldStart = start;
                start = p + 1;
                n = n - (start - oldStart);
                index--;
                break;
            }
            p++;
            i--;
        }
    }
    return json_dumps(res_object, JSON_COMPACT);
}

