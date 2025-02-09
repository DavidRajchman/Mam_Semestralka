#include "reminder.h"

#define REMINDER_NAMESPACE "Rmdr"
static const char *TAG = "REMINDER";

/**
 * @brief Finds and returns the next unused ID in [1..254].
 *        Returns 0 if none found.
 */
static uint8_t find_next_available_id(void)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open_from_partition(NVS_PARTITION, REMINDER_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        // If namespace doesn't exist yet, ID 1 is available.
        return 1;
    }

    // Check each ID until we find a key that doesn't exist
    for (uint8_t i = 1; i < 255; i++) {
        char key[8];
        snprintf(key, sizeof(key), "R%u", i);
        size_t size = 0;
        err = nvs_get_blob(handle, key, NULL, &size);
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            nvs_close(handle);
            return i;
        }
    }
    nvs_close(handle);
    return 0; // No available ID
}

/**
 * @brief Store a new type1 reminder (auto-assigns a free ID).
 *
 * @param reminder Pointer to the reminder data (Reminder_ID should be 0).
 * @param create_even_if_exists If 1, store the reminder even if a reminder with the same Taks_ID, Option ID and additional Option ID exists.
 * @return The assigned reminder ID on success; 0 if store failed.
 */
int store_type1_reminder(const type1_reminder_t *reminder_in, uint8_t create_even_if_exists)
{
    if (!reminder_in) return 0;

    type1_reminder_t new_reminder = *reminder_in;

    // Check for duplicates: same Task_ID, Task_Option_Selected and Task_Additional_Option_Selected.
    if (create_even_if_exists != 1) {
        size_t total = get_num_of_reminders();
        if (total > 0) {
            type1_reminder_t *all_reminders = malloc(total * sizeof(type1_reminder_t));
            if (all_reminders == NULL) {
                ESP_LOGE(TAG, "Memory allocation failed for duplicate check");
                return 0;
            }
            size_t filled = get_all_type1_reminders(all_reminders, total);
            for (size_t i = 0; i < filled; i++) {
                if (all_reminders[i].Task_ID == new_reminder.Task_ID &&
                    all_reminders[i].Task_Option_Selected == new_reminder.Task_Option_Selected &&
                    all_reminders[i].Task_Additional_Option_Selected == new_reminder.Task_Additional_Option_Selected)
                {
                    free(all_reminders);

                        ESP_LOGW(TAG, "Duplicate reminder exists for Task_ID %d, Option %d, Additional Option %d",
                                new_reminder.Task_ID,
                                new_reminder.Task_Option_Selected,
                                new_reminder.Task_Additional_Option_Selected);
                        return -1;
                }
            }
            free(all_reminders);
        }
    }

    uint8_t new_id = find_next_available_id();
    if (new_id == 0) {
        ESP_LOGE(TAG, "No available IDs for new reminder.");
        return 0;
    }
    new_reminder.Reminder_ID = new_id;

    nvs_handle_t handle;
    esp_err_t err = nvs_open_from_partition(NVS_PARTITION, REMINDER_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS for storing reminder: %s", esp_err_to_name(err));
        return 0;
    }

    char key[8];
    snprintf(key, sizeof(key), "R%u", new_id);

    err = nvs_set_blob(handle, key, &new_reminder, sizeof(new_reminder));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set reminder blob: %s", esp_err_to_name(err));
        nvs_close(handle);
        return 0;
    }

    err = nvs_commit(handle);
    nvs_close(handle);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit new reminder: %s", esp_err_to_name(err));
        return 0;
    }
    return new_id;
}

/**
 * @brief Get the number of reminders of any type stored in NVS.
 *
 * @return Number of stored reminders.
 */
size_t get_num_of_reminders(void)
{
    // First, confirm the namespace can be opened (even READONLY).
    // If it doesn't exist, there's nothing to count.
    nvs_handle_t handle;
    esp_err_t err = nvs_open_from_partition(NVS_PARTITION, REMINDER_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        return 0;
    }
    nvs_close(handle);

    size_t count = 0;
    nvs_iterator_t it;
    // Use the new API: nvs_entry_find(part_name, namespace, type, &it)
    esp_err_t it_err = nvs_entry_find(NVS_PARTITION, REMINDER_NAMESPACE, NVS_TYPE_BLOB, &it);
    while (it_err == ESP_OK) {
        count++;
        it_err = nvs_entry_next(&it);
    }
    // Release the iterator once we're done
    nvs_release_iterator(it);

    return count;
}

/**
 * @brief Retrieve all type1 reminders into the given array.
 *
 * @param reminders Pointer to an array of type1_reminder_t.
 * @param max_count Capacity of the provided array.
 * @return Count of reminders placed in the array.
 */
size_t get_all_type1_reminders(type1_reminder_t *reminders, size_t max_count)
{
    if (!reminders || max_count == 0) return 0;

    // Open NVS for reading
    nvs_handle_t handle;
    esp_err_t err = nvs_open_from_partition(NVS_PARTITION, REMINDER_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "No reminders to read or failed to open namespace.");
        return 0;
    }

    size_t num_filled = 0;
    nvs_iterator_t it;
    esp_err_t it_err = nvs_entry_find(NVS_PARTITION, REMINDER_NAMESPACE, NVS_TYPE_BLOB, &it);

    while (it_err == ESP_OK && num_filled < max_count) {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);

        size_t blob_size = sizeof(type1_reminder_t);
        err = nvs_get_blob(handle, info.key, &reminders[num_filled], &blob_size);
        if (err == ESP_OK && blob_size == sizeof(type1_reminder_t)) {
            num_filled++;
        }

        it_err = nvs_entry_next(&it);
    }
    nvs_release_iterator(it);
    nvs_close(handle);

    return num_filled;
}

/**
 * @brief Update an existing type1 reminder in NVS (matching Reminder_ID).
 *
 * @param reminder Pointer to the updated reminder data.
 * @return ESP_OK on success, error otherwise.
 */
esp_err_t update_type1_reminder(const type1_reminder_t *reminder)
{
    if (!reminder || reminder->Reminder_ID == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open_from_partition(NVS_PARTITION, REMINDER_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return err;
    }

    char key[8];
    snprintf(key, sizeof(key), "R%u", reminder->Reminder_ID);

    err = nvs_set_blob(handle, key, reminder, sizeof(*reminder));
    if (err != ESP_OK) {
        nvs_close(handle);
        return err;
    }

    err = nvs_commit(handle);
    nvs_close(handle);

    return err;
}

/**
 * @brief Delete a reminder by its ID.
 *
 * @param reminder_id The ID of the reminder to delete.
 * @return ESP_OK on success, error otherwise.
 */
esp_err_t delete_reminder(uint8_t reminder_id)
{
    if (reminder_id == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open_from_partition(NVS_PARTITION, REMINDER_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return err;
    }

    char key[8];
    snprintf(key, sizeof(key), "R%u", reminder_id);

    err = nvs_erase_key(handle, key);
    if (err != ESP_OK) {
        nvs_close(handle);
        return err;
    }

    err = nvs_commit(handle);
    nvs_close(handle);

    return err;
}

/**
 * @brief Get a type1 reminder by its reminder ID from NVS.
 *
 * @param reminder_id The reminder ID to retrieve.
 * @param reminder Pointer to a type1_reminder_t structure to fill.
 * @return esp_err_t ESP_OK on success, or an error code.
 */
esp_err_t get_reminder_by_id(uint8_t reminder_id, type1_reminder_t *reminder)
{
    if (!reminder || reminder_id == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open_from_partition(NVS_PARTITION, REMINDER_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        return err;
    }

    char key[8];
    snprintf(key, sizeof(key), "R%u", reminder_id);

    size_t blob_size = sizeof(type1_reminder_t);
    err = nvs_get_blob(handle, key, reminder, &blob_size);
    nvs_close(handle);

    if (err != ESP_OK || blob_size != sizeof(type1_reminder_t)) {
        ESP_LOGE(TAG, "Failed to get reminder for key %s: %s", key, esp_err_to_name(err));
        return err;
    }
    return ESP_OK;
}

/**
 * @brief Log the task option string associated with a type1 reminder.
 *
 * This function retrieves the task JSON from NVS using the Task_ID from the reminder,
 * parses it into a task_t structure, and logs the display_text of the selected task option.
 *
 * @param reminder Pointer to a type1_reminder_t structure.
 */
void log_type1_reminder(const type1_reminder_t *reminder)
{
    if (!reminder) {
        ESP_LOGE(TAG, "Invalid reminder pointer");
        return;
    }

    // Retrieve task JSON using Task_ID (stored in reminder->Task_ID)
    char *task_json = retrieve_task_json(reminder->Task_ID);
    if (task_json == NULL) {
        ESP_LOGE(TAG, "Task with ID %d not found in NVS", reminder->Task_ID);
        return;
    }

    task_t task;
    if (!parse_task_json(task_json, &task)) {
        ESP_LOGE(TAG, "Failed to parse task JSON for task ID %d", reminder->Task_ID);
        free(task_json);
        return;
    }
    free(task_json);

    // Validate the selected option index.
    if (reminder->Task_Option_Selected >= TASK_MAX_OPTIONS) {
        ESP_LOGE(TAG, "Invalid task option selected: %d", reminder->Task_Option_Selected);
        return;
    }

    // Log the display text from the selected task option.
    ESP_LOGI(TAG, "Reminder print for Task ID %d, Option %d: %s",
             reminder->Task_ID,
             reminder->Task_Option_Selected,
             task.Options[reminder->Task_Option_Selected].display_text);
}

