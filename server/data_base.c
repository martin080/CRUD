#include "data_base.h"

int init_database()
{
    json_error_t error;
    database = json_load_file(BASE_PATH, 0, &error);
    if (!json_is_array(database))
    {
        fprintf(stderr, "  %s\n", error.text);
        return -1;
    }

    repo_info = json_array_get(database, 0);
    json_t *name = json_object_get(repo_info, "repo_name");
    if (!json_is_string(name))
        return -2;

    json_t *id = json_object_get(repo_info, "cur_available_id");

    if (!json_is_integer(id))
        return -3;

    messageID = json_integer_value(id);

    data_array = json_array_get(database, 1);

    if (!json_is_array(data_array))
        return -4;

    is_repo_default = 1;

    return 0;
}

int dump_database()
{
    json_t *repo_name = json_object_get(repo_info, "repo_name");
    char name_with_extention[128];

    if (!json_is_string(repo_name))
    {
        return -1;
    }

    const char *name_without_extention = json_string_value(repo_name);
    snprintf(name_with_extention, 128, "%s/%s.json", STORAGE_FOLDER, name_without_extention);
    return json_dump_file(database, name_with_extention, 0);
}

int create_repo(const char *name)
{
    static char full_name[128];
    if (strchr(name, '.')) // wrong name
    {
        return -1;
    }

    snprintf(full_name, 128, "%s/%s.json", STORAGE_FOLDER, name);

    int fd = open(full_name, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);

    if (fd == -1) // name is taken
    {
        return -2;
    }

    json_t *repo = json_array(); // init data repo
    json_t *repo_info = json_object();
    json_object_set_new(repo_info, "repo_name", json_string(name));
    json_object_set_new(repo_info, "cur_available_id", json_integer(1));
    json_array_append(repo, repo_info);
    json_array_append_new(repo, json_array());

    json_dumpfd(repo, fd, 0);
    json_decref(repo_info);
    json_decref(repo);

    close(fd);

    return 0;
}

int change_repo(const char *name)
{
    if (strchr(name, '.'))
        return -1;

    static char full_name[128];
    snprintf(full_name, 128, "%s/%s.json", STORAGE_FOLDER, name);

    int fd = open(full_name, O_RDWR);
    if (fd == -1)
        return -2;

    json_t *old_base = database;
    json_error_t error;
    database = json_loadfd(fd, 0, &error);
    if (!json_is_array(database))
    {
        database = old_base;
        return -3;
    }
    json_t *old_repo_info = repo_info;
    repo_info = json_array_get(database, 0);

    if (!json_is_object(repo_info))
    {
        json_decref(database);
        database = old_base;
        json_decref(repo_info);
        repo_info = old_repo_info;
        return -3;
    }

    json_t *id = json_object_get(repo_info, "cur_available_id");

    if (!json_is_integer(id))
    {
        json_decref(database);
        database = old_base;
        json_decref(repo_info);
        repo_info = old_repo_info;
        json_decref(id);
        return -4;
    }

    json_t *repo_name = json_object_get(repo_info, "repo_name");

    if (!json_is_string(repo_name))
    {
        json_decref(database);
        database = old_base;
        json_decref(repo_info);
        repo_info = old_repo_info;
        json_decref(id);
        json_decref(repo_name);
        return -5;
    }

    json_t *old_data_array = data_array;
    data_array = json_array_get(database, 1);

    if (!json_is_array(data_array))
    {
        json_decref(data_array);
        data_array = old_data_array;
        json_decref(repo_info);
        repo_info = old_repo_info;
        json_decref(database);
        database = old_base;
        json_decref(id);
        json_decref(repo_name);
        return -6;
    }

    is_repo_default = (!strcmp(BASE_PATH, full_name) ? 1 : 0); // проблема в том что messageID не обновляется вообще

    messageID = json_integer_value(id);

    return 0;
}

int delete_repo(const char *name)
{
    char full_path[128];
    snprintf(full_path, 128, "%s/%s.json", STORAGE_FOLDER, name);

    int ret = remove(full_path);

    return ret;
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

    if (ret == -1)
        return -1;

    json_t *id = json_object_get(repo_info, "cur_available_id");
    json_integer_set(id, ++messageID);

    return dump_database();
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

            if (ret == -1)
                return -1;

            return dump_database();
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
            return dump_database();
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