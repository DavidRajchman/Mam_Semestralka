#include "json_parser.h"

static const char *TAG = "TT_NVS";


/**
 * @brief Create default timetable structures and store them in NVS.
 *
 * This function creates three default timetables, modifies them to default values, converts
 * them to JSON strings, and stores each under the key "TTX" (where X is the timetable's ID)
 * in the NVS namespace "Jtimetable". Existing entries are overridden.
 */
void set_default_timetables(void)
{
    const char *TAG = "DEFAULT_TIMETABLES";
    timetable_t tt0, tt1, tt2, tt3;
    memset(&tt0, 0, sizeof(tt0));
    memset(&tt1, 0, sizeof(tt1));
    memset(&tt2, 0, sizeof(tt2));
    memset(&tt3, 0, sizeof(tt3));
    // Define default timetable 0 WHICH IS ALWAYS TREATED AS DO NOT DISTURB
    tt0.Type = 1;
    strncpy(tt1.Name, "DO NOT DISTURB", MAX_NAME_LEN);
    tt0.Name[MAX_NAME_LEN] = '\0';
    tt0.ID = 0;
    tt0.times_count = 2;
    tt0.Times_active[0].Start_time = 000;
    tt0.Times_active[0].End_time   = 800;
    tt0.Times_active[1].Start_time = 2200;
    tt0.Times_active[1].End_time   = 2400;
    // Define default timetable 1
    tt1.Type = 1;
    strncpy(tt1.Name, "Morning Schedule", MAX_NAME_LEN);
    tt1.Name[MAX_NAME_LEN] = '\0';
    tt1.ID = 1;
    tt1.times_count = 2;
    tt1.Times_active[0].Start_time = 800;
    tt1.Times_active[0].End_time   = 900;
    tt1.Times_active[1].Start_time = 930;
    tt1.Times_active[1].End_time   = 1000;

    // Define default timetable 2
    tt2.Type = 1;
    strncpy(tt2.Name, "Whole day", MAX_NAME_LEN);
    tt2.Name[MAX_NAME_LEN] = '\0';
    tt2.ID = 2;
    tt2.times_count = 1;
    tt2.Times_active[0].Start_time = 0000;
    tt2.Times_active[0].End_time   = 2400;

    // Define default timetable 3
    tt3.Type = 1;
    strncpy(tt3.Name, "Evening Schedule", MAX_NAME_LEN);
    tt3.Name[MAX_NAME_LEN] = '\0';
    tt3.ID = 3;
    tt3.times_count = 1;
    tt3.Times_active[0].Start_time = 1800;
    tt3.Times_active[0].End_time   = 2100;

    char *json_str;
    esp_err_t err;

    // Convert and store timetable 0
    json_str = timetable_to_json(&tt0);
    if (json_str != NULL) {
        err = store_timetable_json(json_str, true);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Stored default timetable 1 successfully.");
        } else {
            ESP_LOGE(TAG, "Failed to store default timetable 1: %s", esp_err_to_name(err));
        }
        free(json_str);
    } else {
        ESP_LOGE(TAG, "Failed to generate JSON for default timetable 1.");
    }


    // Convert and store timetable 1
    json_str = timetable_to_json(&tt1);
    if (json_str != NULL) {
        err = store_timetable_json(json_str, true);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Stored default timetable 1 successfully.");
        } else {
            ESP_LOGE(TAG, "Failed to store default timetable 1: %s", esp_err_to_name(err));
        }
        free(json_str);
    } else {
        ESP_LOGE(TAG, "Failed to generate JSON for default timetable 1.");
    }

    // Convert and store timetable 2
    json_str = timetable_to_json(&tt2);
    if (json_str != NULL) {
        err = store_timetable_json(json_str, true);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Stored default timetable 2 successfully.");
        } else {
            ESP_LOGE(TAG, "Failed to store default timetable 2: %s", esp_err_to_name(err));
        }
        free(json_str);
    } else {
        ESP_LOGE(TAG, "Failed to generate JSON for default timetable 2.");
    }

    // Convert and store timetable 3
    json_str = timetable_to_json(&tt3);
    if (json_str != NULL) {
        err = store_timetable_json(json_str, true);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Stored default timetable 3 successfully.");
        } else {
            ESP_LOGE(TAG, "Failed to store default timetable 3: %s", esp_err_to_name(err));
        }
        free(json_str);
    } else {
        ESP_LOGE(TAG, "Failed to generate JSON for default timetable 3.");
    }
}

/**
 * @brief Retrieve the timetable JSON string stored in NVS.
 *
 * This function opens the NVS namespace "Jtimetable", constructs the key "TTX" (where X is the provided id),
 * and retrieves the corresponding JSON string. The caller must free the returned string.
 *
 * @param id The timetable ID.
 * @return char* Pointer to the timetable JSON string on success, or NULL on failure.
 */
char *retrieve_timetable_json(int id)
{
    char key[16];
    snprintf(key, sizeof(key), "TT%d", id);

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open_from_partition(NVS_PARTITION,"Jtimetable", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS namespace 'Jtimetable': %s", esp_err_to_name(err));
        return NULL;
    }

    size_t required_size = 0;
    err = nvs_get_str(nvs_handle, key, NULL, &required_size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Timetable with key %s not found", key);
        nvs_close(nvs_handle);
        return NULL;
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error retrieving size for key %s: %s", key, esp_err_to_name(err));
        nvs_close(nvs_handle);
        return NULL;
    }

    char *json_str = malloc(required_size);
    if (!json_str) {
        ESP_LOGE(TAG, "Failed to allocate memory for timetable JSON");
        nvs_close(nvs_handle);
        return NULL;
    }

    err = nvs_get_str(nvs_handle, key, json_str, &required_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error reading timetable JSON for key %s: %s", key, esp_err_to_name(err));
        free(json_str);
        nvs_close(nvs_handle);
        return NULL;
    }

    nvs_close(nvs_handle);
    return json_str;
}

/**
 * @brief Retrieve and log a timetable stored in NVS.
 *
 * This function uses retrieve_timetable_json() to obtain the JSON string corresponding to the provided timetable id,
 * and then logs the JSON string via ESP_LOGI. The allocated JSON string is freed after logging.
 *
 * @param id The timetable ID.
 */
void log_timetable(int id)
{
    char *json_str = retrieve_timetable_json(id);
    if (json_str) {
        ESP_LOGI(TAG, "Timetable with ID %d: %s", id, json_str);
        free(json_str);
    } else {
        ESP_LOGW(TAG, "Timetable with ID %d not found", id);
    }
}


/**
 * @brief Stores a timetable JSON string in NVS under the key "TTX", where X is the timetable ID.
 *
 * The function first opens the NVS namespace "Jtimetable" and parses the JSON string
 * to retrieve the timetable's ID. It then checks if a value is already stored under "TTX".
 * If a value exists and override_timetable is false, it logs a warning and returns an error.
 * If override_timetable is true, or no entry exists, it stores the JSON string and commits it.
 *
 * @param json_str The JSON string representing the timetable.
 * @param override_timetable If true, existing value is overwritten.
 * @return esp_err_t ESP_OK if successful, or an ESP error code on failure.
 */
esp_err_t store_timetable_json(const char *json_str, bool override_timetable)
{
    const char *TAG = "TT_NVS";
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open_from_partition(NVS_PARTITION,"Jtimetable", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS namespace 'Jtimetable': %s", esp_err_to_name(err));
        return err;
    }

    // Parse the JSON to retrieve timetable ID.
    timetable_t timetable;
    if (!parse_timetable_json(json_str, &timetable)) {
        ESP_LOGE(TAG, "Failed to parse timetable JSON.");
        nvs_close(nvs_handle);
        return ESP_ERR_INVALID_ARG;
    }

    // Construct the key "TTX" where X is timetable.ID.
    char key[16];
    snprintf(key, sizeof(key), "TT%d", timetable.ID);

    // Check if a value is already stored under key.
    size_t required_size = 0;
    err = nvs_get_str(nvs_handle, key, NULL, &required_size);
    if (err == ESP_OK) {
        // Key exists.
        if (!override_timetable) {
            ESP_LOGW(TAG, "Key %s already exists and override is disabled.", key);
            nvs_close(nvs_handle);
            return ESP_ERR_NVS_INVALID_STATE;
        }
    } else if (err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Error checking for key %s: %s", key, esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    // Write the JSON string under the key.
    err = nvs_set_str(nvs_handle, key, json_str);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error writing JSON to NVS under key %s: %s", key, esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    // Commit the change.
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error committing NVS changes for key %s: %s", key, esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Successfully stored timetable under key %s", key);
    }

    nvs_close(nvs_handle);
    return err;
}

/**
 * @brief Parse a timetable JSON string and fill the timetable structure.
 *
 * This function parses a JSON text representing a timetable configuration and
 * stores the values in a timetable_t struct. On error, it logs the reason and returns false.
 *
 * @param json_str The JSON string.
 * @param timetable Pointer to the timetable_t struct to fill.
 * @return true if parsing is successful, false otherwise.
 */
bool parse_timetable_json(const char *json_str, timetable_t *timetable)
{
    const char *TAG = "JSON_timetable_parse_json_string";

    // Validate input pointers.
    if (!json_str || !timetable) {
        ESP_LOGE(TAG, "Invalid input: json_str or timetable pointer is NULL");
        return false;
    }

    // Parse the JSON string.
    cJSON *root = cJSON_Parse(json_str);
    if (!root) {
        ESP_LOGE(TAG, "Failed to parse JSON string");
        return false;
    }

    // Get the "Type" field.
    cJSON *type = cJSON_GetObjectItem(root, "Type");
    if (cJSON_IsNumber(type))
        timetable->Type = (uint8_t)type->valueint;
    else {
        ESP_LOGE(TAG, "Invalid or missing 'Type' field in JSON");
        cJSON_Delete(root);
        return false;
    }

    // Get the "Name" field.
    cJSON *name = cJSON_GetObjectItem(root, "Name");
    if (cJSON_IsString(name)) {
        strncpy(timetable->Name, name->valuestring, MAX_NAME_LEN);
        timetable->Name[MAX_NAME_LEN] = '\0';  // Ensure null termination.
    } else {
        ESP_LOGE(TAG, "Invalid or missing 'Name' field in JSON");
        cJSON_Delete(root);
        return false;
    }

    // Get the "ID" field.
    cJSON *id = cJSON_GetObjectItem(root, "ID");
    if (cJSON_IsNumber(id))
        timetable->ID = (uint8_t)id->valueint;
    else {
        ESP_LOGE(TAG, "Invalid or missing 'ID' field in JSON");
        cJSON_Delete(root);
        return false;
    }

    // Get the "Times_active" array.
    cJSON *times_active = cJSON_GetObjectItem(root, "Times_active");
    if (cJSON_IsArray(times_active)) {
        int slot_count = cJSON_GetArraySize(times_active);
        if (slot_count > MAX_TIMESLOTS)
            slot_count = MAX_TIMESLOTS;
        timetable->times_count = (uint8_t)slot_count;

        // Iterate through each timeslot object.
        for (int i = 0; i < slot_count; i++) {
            cJSON *slot = cJSON_GetArrayItem(times_active, i);
            if (slot) {
                cJSON *start_time = cJSON_GetObjectItem(slot, "Start_time");
                cJSON *end_time = cJSON_GetObjectItem(slot, "End_time");
                if (cJSON_IsNumber(start_time) && cJSON_IsNumber(end_time)) {
                    timetable->Times_active[i].Start_time = (uint16_t)start_time->valueint;
                    timetable->Times_active[i].End_time = (uint16_t)end_time->valueint;
                } else {
                    ESP_LOGE(TAG, "Invalid or missing 'Start_time' or 'End_time' in timeslot %d", i);
                    cJSON_Delete(root);
                    return false;
                }
            } else {
                ESP_LOGE(TAG, "Missing timeslot at index %d", i);
                cJSON_Delete(root);
                return false;
            }
        }
    } else {
        ESP_LOGE(TAG, "Invalid or missing 'Times_active' array in JSON");
        cJSON_Delete(root);
        return false;
    }

    cJSON_Delete(root);
    return true;
}

/**
 * @brief Convert a timetable structure to its JSON representation.
 *
 * This function creates a JSON string from the given timetable_t struct.
 * The returned string must be freed by the caller using free().
 *
 * @param timetable Pointer to the timetable_t struct.
 * @return char* JSON string on success, or NULL on failure.
 */
char *timetable_to_json(const timetable_t *timetable)
{
    const char *TAG = "JSON_timetable_to_json_string";
    if (!timetable) {
        ESP_LOGE(TAG, "Invalid input: timetable pointer is NULL");
        return NULL;
    }

    cJSON *root = cJSON_CreateObject();
    if (!root) {
        ESP_LOGE(TAG, "Failed to create JSON root object");
        return NULL;
    }

    // Add fields to JSON.
    if (!cJSON_AddNumberToObject(root, "Type", timetable->Type)) {
        ESP_LOGE(TAG, "Failed to add 'Type' to JSON");
        cJSON_Delete(root);
        return NULL;
    }

    if (!cJSON_AddStringToObject(root, "Name", timetable->Name)) {
        ESP_LOGE(TAG, "Failed to add 'Name' to JSON");
        cJSON_Delete(root);
        return NULL;
    }

    if (!cJSON_AddNumberToObject(root, "ID", timetable->ID)) {
        ESP_LOGE(TAG, "Failed to add 'ID' to JSON");
        cJSON_Delete(root);
        return NULL;
    }

    // Create and add the "Times_active" array.
    cJSON *times_active = cJSON_CreateArray();
    if (!times_active) {
        ESP_LOGE(TAG, "Failed to create 'Times_active' array");
        cJSON_Delete(root);
        return NULL;
    }
    cJSON_AddItemToObject(root, "Times_active", times_active);

    for (int i = 0; i < timetable->times_count; i++) {
        cJSON *slot = cJSON_CreateObject();
        if (!slot) {
            ESP_LOGE(TAG, "Failed to create timeslot object at index %d", i);
            cJSON_Delete(root);
            return NULL;
        }
        cJSON_AddItemToArray(times_active, slot);

        if (!cJSON_AddNumberToObject(slot, "Start_time", timetable->Times_active[i].Start_time)) {
            ESP_LOGE(TAG, "Failed to add 'Start_time' for timeslot %d", i);
            cJSON_Delete(root);
            return NULL;
        }

        if (!cJSON_AddNumberToObject(slot, "End_time", timetable->Times_active[i].End_time)) {
            ESP_LOGE(TAG, "Failed to add 'End_time' for timeslot %d", i);
            cJSON_Delete(root);
            return NULL;
        }
    }

    // Convert the cJSON object to a string.
    char *json_str = cJSON_PrintUnformatted(root);
    if (!json_str) {
        ESP_LOGE(TAG, "Failed to print JSON string");
        cJSON_Delete(root);
        return NULL;
    }

    cJSON_Delete(root);
    return json_str;
}