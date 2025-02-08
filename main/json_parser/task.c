#include "task.h"
#include "json_parser.h"  // Contains cJSON and esp logging includes

// Use a dedicated tag for logging.
static const char *TAG = "TASK_NVS";


/**
 * @brief Create default task(s) and store them in NVS.
 *
 * Creates default tasks without RFID_UID (empty string) and stores them.
 */
void set_default_tasks(void)
{
    const char *localTAG = "DEFAULT_TASKS";
    // Create three default tasks.
    task_t t1, t2, t3;
    memset(&t1, 0, sizeof(t1));
    memset(&t2, 0, sizeof(t2));
    memset(&t3, 0, sizeof(t3));

    // Default Task 1
    t1.Type = 1;
    strncpy(t1.Name, "Cas Doma", MAX_TASK_NAME_LEN);
    t1.Name[MAX_TASK_NAME_LEN] = '\0';
    t1.ID = 1;
    //rfid uid = AAB4B512
    strncpy(t1.RFID_UID, "AAB4B512", MAX_RFID_UID_LEN);
    // Fill options (always 4 options)
    for (int i = 0; i < TASK_MAX_OPTIONS; i++) {
        // For default, you might vary details; here we fill two options and leave others empty.
        if(i == 0) {
            strncpy(t1.Options[i].display_text, "breakfast 1d", MAX_OPTION_DISPLAY_LEN);
            t1.Options[i].display_text[MAX_OPTION_DISPLAY_LEN] = '\0';
            uint8_t ts1[] = {1, 2, 4};
            t1.Options[i].timeslot_count = sizeof(ts1) / sizeof(ts1[0]);
            for (int j = 0; j < t1.Options[i].timeslot_count; j++) {
                t1.Options[i].timeslots[j] = ts1[j];
            }
            t1.Options[i].priority = 1;
            t1.Options[i].days_till_em = 1;
        } else if(i == 1) {
            strncpy(t1.Options[i].display_text, "dinner 1d", MAX_OPTION_DISPLAY_LEN);
            t1.Options[i].display_text[MAX_OPTION_DISPLAY_LEN] = '\0';
            uint8_t ts2[] = {1, 3};
            t1.Options[i].timeslot_count = sizeof(ts2) / sizeof(ts2[0]);
            for (int j = 0; j < t1.Options[i].timeslot_count; j++) {
                t1.Options[i].timeslots[j] = ts2[j];
            }
            t1.Options[i].priority = 1;
            t1.Options[i].days_till_em = 1;
        } else {
            // Empty option
            t1.Options[i].display_text[0] = '\0';
            t1.Options[i].timeslot_count = 0;
            t1.Options[i].priority = 0;
            t1.Options[i].days_till_em = 0;
        }
    }

    // Default Task 2
    t2.Type = 1;
    strncpy(t2.Name, "Work Task", MAX_TASK_NAME_LEN);
    t2.Name[MAX_TASK_NAME_LEN] = '\0';
    t2.ID = 2;
    t2.RFID_UID[0] = '\0';
    for (int i = 0; i < TASK_MAX_OPTIONS; i++) {
        if(i == 0) {
            strncpy(t2.Options[i].display_text, "Email Check", MAX_OPTION_DISPLAY_LEN);
            t2.Options[i].display_text[MAX_OPTION_DISPLAY_LEN] = '\0';
            t2.Options[i].timeslot_count = 0;
            t2.Options[i].priority = 1;
            t2.Options[i].days_till_em = 0;
        } else {
            t2.Options[i].display_text[0] = '\0';
            t2.Options[i].timeslot_count = 0;
            t2.Options[i].priority = 0;
            t2.Options[i].days_till_em = 0;
        }
    }

    // Default Task 3
    const char *t3_json = "{"
                            "\"Type\": 1,"
                            "\"Name\": \"My Custom Task\","
                            "\"ID\": 3,"
                            "\"RFID_UID\": \"47F13834\","
                            "\"Options\": ["
                                "{"
                                    "\"display_text\": \"Option 1: Description\","
                                    "\"Timeslots\": [2,3],"
                                    "\"priority\": 2,"
                                    "\"days_till_em\": 0"
                                "},"
                                "{"
                                    "\"display_text\": \"Option 2: Description\","
                                    "\"Timeslots\": [1],"
                                    "\"priority\": 1,"
                                    "\"days_till_em\": 0"
                                "},"
                                "{"
                                    "\"display_text\": \"Option 3: Description\","
                                    "\"Timeslots\": [],"
                                    "\"priority\": 0,"
                                    "\"days_till_em\": 0"
                                "},"
                                "{"
                                    "\"display_text\": \"Option 4: Description\","
                                    "\"Timeslots\": [],"
                                    "\"priority\": 0,"
                                    "\"days_till_em\": 0"
                                "}"
                            "]"
                          "}";

    // Store t3 using the JSON string.
    esp_err_t err = store_task_json(t3_json, true);
    if(err == ESP_OK)
        ESP_LOGI(localTAG, "Stored default task 3 (personal task) successfully.");
    else
        ESP_LOGE(localTAG, "Failed to store default task 3: %s", esp_err_to_name(err));

    char *json_str;

    // Store each task using task_to_json and store_task_json.
    json_str = task_to_json(&t1);
    if (json_str != NULL) {
        err = store_task_json(json_str, true);
        if(err == ESP_OK)
            ESP_LOGI(localTAG, "Stored default task 1 successfully.");
        else
            ESP_LOGE(localTAG, "Failed to store default task 1: %s", esp_err_to_name(err));
        free(json_str);
    } else {
        ESP_LOGE(localTAG, "Failed to generate JSON for default task 1.");
    }

    json_str = task_to_json(&t2);
    if (json_str != NULL) {
        err = store_task_json(json_str, true);
        if(err == ESP_OK)
            ESP_LOGI(localTAG, "Stored default task 2 successfully.");
        else
            ESP_LOGE(localTAG, "Failed to store default task 2: %s", esp_err_to_name(err));
        free(json_str);
    } else {
        ESP_LOGE(localTAG, "Failed to generate JSON for default task 2.");
    }


}

/**
 * @brief Parse a task JSON string and fill the task structure.
 *
 * Expected JSON format:
 * {
 *   "Type": 1,
 *   "Name": "Cas Doma",
 *   "ID": 1,
 *   "RFID_UID": "AA BB CC DD",
 *   "Options": [
 *       {
 *           "display_text": "breakfast 1d",
 *           "Timeslots": [1,2,4],
 *           "priority": 1,
 *           "days_till_em": 1
 *       },
 *       { ... },  // exactly 4 options expected
 *       { ... },
 *       { ... }
 *   ]
 * }
 */
bool parse_task_json(const char *json_str, task_t *task)
{
    if (!json_str || !task) {
        ESP_LOGE(TAG, "Invalid input to parse_task_json");
        return false;
    }
    cJSON *root = cJSON_Parse(json_str);
    if (!root) {
        ESP_LOGE(TAG, "Failed to parse task JSON string");
        return false;
    }

    // Parse "Type"
    cJSON *type = cJSON_GetObjectItem(root, "Type");
    if (cJSON_IsNumber(type))
        task->Type = (uint8_t)type->valueint;
    else {
        ESP_LOGE(TAG, "Missing or invalid 'Type'");
        cJSON_Delete(root);
        return false;
    }

    // Parse "Name"
    cJSON *name = cJSON_GetObjectItem(root, "Name");
    if (cJSON_IsString(name)) {
        strncpy(task->Name, name->valuestring, MAX_TASK_NAME_LEN);
        task->Name[MAX_TASK_NAME_LEN] = '\0';
    } else {
        ESP_LOGE(TAG, "Missing or invalid 'Name'");
        cJSON_Delete(root);
        return false;
    }

    // Parse "ID"
    cJSON *id = cJSON_GetObjectItem(root, "ID");
    if (cJSON_IsNumber(id))
        task->ID = (uint8_t)id->valueint;
    else {
        ESP_LOGE(TAG, "Missing or invalid 'ID'");
        cJSON_Delete(root);
        return false;
    }

    // Parse "RFID_UID"
    cJSON *rfid = cJSON_GetObjectItem(root, "RFID_UID");
    if (cJSON_IsString(rfid)) {
        strncpy(task->RFID_UID, rfid->valuestring, MAX_RFID_UID_LEN);
        task->RFID_UID[MAX_RFID_UID_LEN] = '\0';
    } else {
        // If missing, set to empty string
        task->RFID_UID[0] = '\0';
    }

    // Parse "Options" array. Expect exactly 4.
    cJSON *options = cJSON_GetObjectItem(root, "Options");
    if (!cJSON_IsArray(options) || (cJSON_GetArraySize(options) != TASK_MAX_OPTIONS)) {
        ESP_LOGE(TAG, "'Options' must be an array of exactly %d items", TASK_MAX_OPTIONS);
        cJSON_Delete(root);
        return false;
    }

    for (int i = 0; i < TASK_MAX_OPTIONS; i++) {
        cJSON *option = cJSON_GetArrayItem(options, i);
        if (!option) {
            ESP_LOGE(TAG, "Missing option at index %d", i);
            cJSON_Delete(root);
            return false;
        }

        // Parse "display_text"
        cJSON *disp = cJSON_GetObjectItem(option, "display_text");
        if (cJSON_IsString(disp)) {
            strncpy(task->Options[i].display_text, disp->valuestring, MAX_OPTION_DISPLAY_LEN);
            task->Options[i].display_text[MAX_OPTION_DISPLAY_LEN] = '\0';
        } else {
            ESP_LOGE(TAG, "Missing or invalid 'display_text' in option %d", i);
            cJSON_Delete(root);
            return false;
        }

        // Parse "Timeslots" array (can be empty)
        cJSON *ts_array = cJSON_GetObjectItem(option, "Timeslots");
        if (ts_array && cJSON_IsArray(ts_array)) {
            int ts_count = cJSON_GetArraySize(ts_array);
            if (ts_count > MAX_TASK_TIMESLOTS)
                ts_count = MAX_TASK_TIMESLOTS;
            task->Options[i].timeslot_count = (uint8_t)ts_count;
            for (int j = 0; j < ts_count; j++) {
                cJSON *ts_item = cJSON_GetArrayItem(ts_array, j);
                if (cJSON_IsNumber(ts_item))
                    task->Options[i].timeslots[j] = (uint8_t)ts_item->valueint;
                else {
                    ESP_LOGE(TAG, "Invalid timeslot in option %d index %d", i, j);
                    cJSON_Delete(root);
                    return false;
                }
            }
        } else {
            // if not present, set count to 0.
            task->Options[i].timeslot_count = 0;
        }

        // Parse "priority"
        cJSON *priority = cJSON_GetObjectItem(option, "priority");
        if (cJSON_IsNumber(priority))
            task->Options[i].priority = (int8_t)priority->valueint;
        else {
            ESP_LOGE(TAG, "Missing or invalid 'priority' in option %d", i);
            cJSON_Delete(root);
            return false;
        }

        // Parse "days_till_em"
        cJSON *days = cJSON_GetObjectItem(option, "days_till_em");
        if (cJSON_IsNumber(days))
            task->Options[i].days_till_em = (int8_t)days->valueint;
        else {
            ESP_LOGE(TAG, "Missing or invalid 'days_till_em' in option %d", i);
            cJSON_Delete(root);
            return false;
        }
    }

    cJSON_Delete(root);
    return true;
}

/**
 * @brief Convert a task structure to its JSON representation.
 * The returned JSON string must be freed by the caller.
 */
char *task_to_json(const task_t *task)
{
    if (!task) {
        ESP_LOGE(TAG, "Invalid task pointer in task_to_json");
        return NULL;
    }
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        ESP_LOGE(TAG, "Failed to create JSON root");
        return NULL;
    }

    if (!cJSON_AddNumberToObject(root, "Type", task->Type)) {
        ESP_LOGE(TAG, "Failed to add 'Type'");
        cJSON_Delete(root);
        return NULL;
    }

    if (!cJSON_AddStringToObject(root, "Name", task->Name)) {
        ESP_LOGE(TAG, "Failed to add 'Name'");
        cJSON_Delete(root);
        return NULL;
    }

    if (!cJSON_AddNumberToObject(root, "ID", task->ID)) {
        ESP_LOGE(TAG, "Failed to add 'ID'");
        cJSON_Delete(root);
        return NULL;
    }

    if (!cJSON_AddStringToObject(root, "RFID_UID", task->RFID_UID)) {
        ESP_LOGE(TAG, "Failed to add 'RFID_UID'");
        cJSON_Delete(root);
        return NULL;
    }

    // Create Options array
    cJSON *options = cJSON_CreateArray();
    if (!options) {
        ESP_LOGE(TAG, "Failed to create Options array");
        cJSON_Delete(root);
        return NULL;
    }
    cJSON_AddItemToObject(root, "Options", options);

    for (int i = 0; i < TASK_MAX_OPTIONS; i++) {
        cJSON *option = cJSON_CreateObject();
        if (!option) {
            ESP_LOGE(TAG, "Failed to create option object at index %d", i);
            cJSON_Delete(root);
            return NULL;
        }
        cJSON_AddItemToArray(options, option);

        if (!cJSON_AddStringToObject(option, "display_text", task->Options[i].display_text)) {
            ESP_LOGE(TAG, "Failed to add 'display_text' for option %d", i);
            cJSON_Delete(root);
            return NULL;
        }

        // Create Timeslots array
        cJSON *ts_array = cJSON_CreateArray();
        if (!ts_array) {
            ESP_LOGE(TAG, "Failed to create Timeslots array for option %d", i);
            cJSON_Delete(root);
            return NULL;
        }
        cJSON_AddItemToObject(option, "Timeslots", ts_array);
        for (int j = 0; j < task->Options[i].timeslot_count; j++) {
            cJSON *ts_item = cJSON_CreateNumber(task->Options[i].timeslots[j]);
            if (!ts_item) {
                ESP_LOGE(TAG, "Failed to create timeslot number for option %d index %d", i, j);
                cJSON_Delete(root);
                return NULL;
            }
            cJSON_AddItemToArray(ts_array, ts_item);
        }

        if (!cJSON_AddNumberToObject(option, "priority", task->Options[i].priority)) {
            ESP_LOGE(TAG, "Failed to add 'priority' for option %d", i);
            cJSON_Delete(root);
            return NULL;
        }

        if (!cJSON_AddNumberToObject(option, "days_till_em", task->Options[i].days_till_em)) {
            ESP_LOGE(TAG, "Failed to add 'days_till_em' for option %d", i);
            cJSON_Delete(root);
            return NULL;
        }
    }

    char *json_str = cJSON_PrintUnformatted(root);
    if (!json_str) {
        ESP_LOGE(TAG, "Failed to print task JSON string");
        cJSON_Delete(root);
        return NULL;
    }

    cJSON_Delete(root);
    return json_str;
}

/**
 * @brief Stores a task JSON string in NVS.
 *
 * Opens namespace "Jtask" and uses as key  "T<ID>".
 * If rfid_uid is not empty, it calls assign_rfid_to_task.
 */
esp_err_t store_task_json(const char *json_str, bool override_task)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open_from_partition(NVS_PARTITION,"Jtask", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS namespace 'Jtask': %s", esp_err_to_name(err));
        return err;
    }

    task_t task;
    if (!parse_task_json(json_str, &task)) {
        ESP_LOGE(TAG, "Failed to parse task JSON");
        nvs_close(nvs_handle);
        return ESP_ERR_INVALID_ARG;
    }

    char key[32];
    char RFID_UID[17];
    uint8_t rfid_key_found = 0;
    snprintf(key, sizeof(key), "T%d", task.ID);

    if (task.RFID_UID[0] != '\0') {

        strncpy(RFID_UID, task.RFID_UID, 16);
        RFID_UID[16] = '\0';
        rfid_key_found = 1;

    } else {
    }

    size_t required_size = 0;
    err = nvs_get_str(nvs_handle, key, NULL, &required_size);
    if (err == ESP_OK) {
        if (!override_task) {
            ESP_LOGW(TAG, "Key %s already exists and override is disabled", key);
            nvs_close(nvs_handle);
            return ESP_ERR_NVS_INVALID_STATE;
        }
    } else if (err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Error checking key %s: %s", key, esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    err = nvs_set_str(nvs_handle, key, json_str);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error writing JSON to NVS key %s: %s", key, esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error committing NVS changes for key %s: %s", key, esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Successfully stored task under key %s", key);
    }
    if (rfid_key_found) {
        err = assign_rfid_to_task(RFID_UID, task.ID);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to assign RFID to task: %s", esp_err_to_name(err));
        }
        else {
            ESP_LOGI(TAG, "Assigned RFID to task successfully");
        }
    }
    nvs_close(nvs_handle);
    return err;
}

/**
 * @brief Assign an RFID UID to a task.
 *
 * Loads the current mapping blob from NVS, updates or inserts the mapping
 * for the provided RFID UID with the specified task ID, and writes the blob back.
 *
 * @param rfid_uid The RFID UID string (16 hex characters).
 * @param task_id The task ID to assign to the RFID UID.
 * @return esp_err_t ESP_OK on success, otherwise an error code.
 */
esp_err_t assign_rfid_to_task(const char *rfid_uid, uint8_t task_id)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open_from_partition(NVS_PARTITION,RFID_MAP_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS namespace for RFID mapping: %s", esp_err_to_name(err));
        return err;
    }

    rfid_mapping_t mapping;
    size_t required_size = sizeof(mapping);

    // Try to read the blob. If not found, initialize mapping.
    err = nvs_get_blob(nvs_handle, RFID_MAP_NVS_KEY, &mapping, &required_size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        memset(&mapping, 0, sizeof(mapping));
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get RFID mapping blob: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    // Search for an existing entry with this RFID UID.
    bool found = false;
    for (int i = 0; i < mapping.count; i++) {
        if (strncmp(mapping.entries[i].rfid_uid, rfid_uid, 16) == 0) {
            mapping.entries[i].task_id = task_id;
            found = true;
            break;
        }
    }

    // If not found and there is room, add it.
    if (!found) {
        if (mapping.count < MAX_RFID_MAPPINGS) {
            strncpy(mapping.entries[mapping.count].rfid_uid, rfid_uid, 16);
            mapping.entries[mapping.count].rfid_uid[16] = '\0';
            mapping.entries[mapping.count].task_id = task_id;
            mapping.count++;
        } else {
            ESP_LOGE(TAG, "RFID mapping is full");
            nvs_close(nvs_handle);
            return ESP_ERR_NO_MEM;
        }
    }

    // Write the mapping blob back.
    err = nvs_set_blob(nvs_handle, RFID_MAP_NVS_KEY, &mapping, sizeof(mapping));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write RFID mapping blob: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }
    err = nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    return err;
}

/**
 * @brief Retrieve the task ID assigned to a given RFID UID.
 *
 * Opens the mapping blob from NVS and searches for the provided RFID UID.
 *
 * @param rfid_uid The RFID UID string.
 * @return int The task ID if found, or -1 if not found or on error.
 */
int get_task_id_by_rfid(const char *rfid_uid)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open_from_partition(NVS_PARTITION,RFID_MAP_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS namespace for RFID mapping: %s", esp_err_to_name(err));
        return -1;
    }

    rfid_mapping_t mapping;
    size_t required_size = sizeof(mapping);
    err = nvs_get_blob(nvs_handle, RFID_MAP_NVS_KEY, &mapping, &required_size);
    nvs_close(nvs_handle);

    if (err != ESP_OK) {
        ESP_LOGW(TAG, "RFID mapping blob not found");
        return -1;
    }

    for (int i = 0; i < mapping.count; i++) {
        if (strncmp(mapping.entries[i].rfid_uid, rfid_uid, 16) == 0) {
            return mapping.entries[i].task_id;
        }
    }
    return -1; // Not found.
}


/**
 * @brief Retrieves the task JSON string from NVS by ID.
 *
 * This function looks for key "T<ID>".
 */
char *retrieve_task_json(int id)
{
    char key[32];
    snprintf(key, sizeof(key), "T%d", id);
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open_from_partition(NVS_PARTITION,"Jtask", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS namespace 'Jtask': %s", esp_err_to_name(err));
        return NULL;
    }

    size_t required_size = 0;
    err = nvs_get_str(nvs_handle, key, NULL, &required_size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Task with key %s not found", key);
        nvs_close(nvs_handle);
        return NULL;
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error reading key %s: %s", key, esp_err_to_name(err));
        nvs_close(nvs_handle);
        return NULL;
    }

    char *json_str = malloc(required_size);
    if (!json_str) {
        ESP_LOGE(TAG, "Failed to allocate memory for task JSON");
        nvs_close(nvs_handle);
        return NULL;
    }

    err = nvs_get_str(nvs_handle, key, json_str, &required_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error retrieving task JSON for key %s: %s", key, esp_err_to_name(err));
        free(json_str);
        nvs_close(nvs_handle);
        return NULL;
    }
    nvs_close(nvs_handle);
    return json_str;
}


/**
 * @brief Retrieve and log a task stored in NVS by ID.
 */
void log_task(int id)
{
    char *json_str = retrieve_task_json(id);
    if (json_str) {
        ESP_LOGI(TAG, "Task with ID %d: %s", id, json_str);
        free(json_str);
    } else {
        ESP_LOGW(TAG, "Task with ID %d not found", id);
    }
}

