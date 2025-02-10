#include "alarm_execution.h"
// for get_task_by_id, task_t and task_option_t

static const char *TAG = "alarm_execution";


//---------------------------------------------------------------------
// Module-level static pointers set during initialization.
static QueueHandle_t my_chirpQueue    = NULL;
static SemaphoreHandle_t my_display_mutex = NULL;
static u8g2_t *my_u8g2_ptr           = NULL;

//---------------------------------------------------------------------
// Priority configuration structure for LED/chirp/display settings.
typedef struct {
    uint8_t default_led_red;
    uint8_t default_led_green;
    uint8_t default_led_blue;
    uint8_t default_chirp_id;
    uint32_t display_interval_minutes; // interval between message displays
} priority_config_t;

/**
 * @brief Returns a pointer to an array of three configurations used for LED, chirp, and display.
 *
 * Even though the emergency threshold is loaded from the task option,
 * these defaults are used for actions (indexed by task option priority, where 1 is highest).
 *
 * @return Pointer to an array of priority_config_t of size 3.
 */
static const priority_config_t* get_priority_configs(void)
{
    static const priority_config_t configs[3] = {
        { .default_led_red = 0,   .default_led_green = 255, .default_led_blue = 0,   .default_chirp_id = 4, .display_interval_minutes = 15 },
        { .default_led_red = 0,   .default_led_green = 0,   .default_led_blue = 255, .default_chirp_id = 4, .display_interval_minutes = 10 },
        { .default_led_red = 255, .default_led_green = 0,   .default_led_blue = 0,   .default_chirp_id = 4, .display_interval_minutes = 5  }
    };
    ESP_LOGI(TAG, "Using default priority configurations");
    return configs;
}

//---------------------------------------------------------------------
// Helper functions assume that timetable_t and type1_reminder_t are defined in json_parser.h.

// Checks if the given timetable has any active timeslot for the current time.
static bool check_timeslot_active(const timetable_t *timetable)
{
    if(timetable == NULL) return false;
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    if(tm_info == NULL) return false;
    int current_time = tm_info->tm_hour * 100 + tm_info->tm_min;
    for (int i = 0; i < timetable->times_count; i++) {
        if (current_time >= timetable->Times_active[i].Start_time &&
            current_time < timetable->Times_active[i].End_time) {
            ESP_LOGI(TAG, "Current time (%d) within timeslot [%d, %d]", current_time,
                     timetable->Times_active[i].Start_time, timetable->Times_active[i].End_time);
            return true;
        }
    }
    return false;
}

/**
 * Loads a timetable using retrieve_timetable_json and parse_timetable_json.
 *
 * @param id        Timetable id.
 * @param timetable Pointer to timetable_t to fill.
 * @return true on success, false otherwise.
 */
static bool load_timetable(int id, timetable_t *timetable)
{
    ESP_LOGI(TAG, "Loading timetable for id %d", id);
    char *json_str = retrieve_timetable_json(id);
    if (json_str == NULL) {
        ESP_LOGW(TAG, "No timetable found for id %d", id);
        return false;
    }
    bool res = parse_timetable_json(json_str, timetable);
    free(json_str);
    ESP_LOGI(TAG, "Timetable id %d loaded: %s", id, res ? "success" : "failure");
    return res;
}

/**
 * Checks all reminders and returns the highest priority active reminder.
 *
 * For each reminder, its associated task is loaded using get_task_by_id.
 * Using the reminder’s Task_Option_Selected, the emergency threshold (days_till_em)
 * and the option priority are loaded.
 * A reminder is active if its timetable is active or if the elapsed days have reached
 * the emergency threshold. If the reminder is in emergency status it is assigned
 * the EMERGENCY_PRIORITY.
 *
 * @param active_priority Output: highest active priority (lower value means higher priority).
 *                        Set to 0 if none active.
 * @param active_reminder Output: first active reminder with the highest priority.
 * @return true if an active reminder is found, false otherwise.
 */
static bool check_active_reminders(uint8_t *active_priority, type1_reminder_t *active_reminder)
{
    task_t *task = NULL;
    ESP_LOGI(TAG, "Checking active reminders");
    size_t num = get_num_of_reminders();
    if(num == 0) {
        ESP_LOGI(TAG, "No reminders found");
        return false;
    }

    type1_reminder_t *all = malloc(num * sizeof(type1_reminder_t));
    if(all == NULL) {
        ESP_LOGE(TAG, "Memory allocation failed for reminders");
        return false;
    }
    num = get_all_type1_reminders(all, num);
    ESP_LOGI(TAG, "Found %d reminder(s)", (int)num);

    const priority_config_t *default_configs = get_priority_configs();
    bool found = false;
    uint8_t current_best_priority = 0;
    time_t now = time(NULL);

    for (size_t i = 0; i < num; i++) {
        ESP_LOGI(TAG, "Processing reminder with ID %d", all[i].Reminder_ID);
        //get task from reminder
        uint8_t task_option_selected = all[i].Task_Option_Selected;
        task = get_task_by_id(all[i].Task_ID);

        //get number of timetables
        uint8_t num_timetables = task->Options[task_option_selected].timeslot_count;
        ESP_LOGI(TAG, "Reminder %d has %d timetables", all[i].Reminder_ID, num_timetables);
        //go through all timetables to find 1 that is active
        for(uint8_t j = 0; j < num_timetables; j++) {
            uint8_t timetableID = task->Options[task_option_selected].timeslots[j];
            ESP_LOGI(TAG, "Reminder %d timetable %d: %d", all[i].Reminder_ID, j, timetableID);
            timetable_t timetable;
            bool timetable_loaded = load_timetable(timetableID, &timetable);

            bool active_in_timeslot = timetable_loaded && check_timeslot_active(&timetable);
            if (!timetable_loaded) {
                ESP_LOGW(TAG, "Timetable not loaded for reminder %d", all[i].Reminder_ID);
            }
            // If the timetable is loaded, additionally log the active timeslot(s) for this reminder.
            if(timetable_loaded) {
                struct tm *tm_info = localtime(&now);
                if(tm_info != NULL) {
                    int current_time = tm_info->tm_hour * 100 + tm_info->tm_min;
                    for (int j = 0; j < timetable.times_count; j++) {
                        if (current_time >= timetable.Times_active[j].Start_time &&
                            current_time < timetable.Times_active[j].End_time) {
                            ESP_LOGI(TAG, "Reminder %d active timeslot: [%d, %d]",
                                     all[i].Reminder_ID,
                                     timetable.Times_active[j].Start_time,
                                     timetable.Times_active[j].End_time);
                        }
                        else {
                            ESP_LOGI(TAG, "Reminder %d inactive timeslot: [%d, %d]",
                                     all[i].Reminder_ID,
                                     timetable.Times_active[j].Start_time,
                                     timetable.Times_active[j].End_time);
                        }
                    }
                }
            }

            int option_threshold = 0; //treshold of days til emergency
            int option_priority = 255;
            if (task != NULL) {
                int opt_index = all[i].Task_Option_Selected;
                if(opt_index >= 0 && opt_index < TASK_MAX_OPTIONS) {
                    option_threshold = task->Options[opt_index].days_till_em;
                    option_priority = task->Options[opt_index].priority;
                    ESP_LOGI(TAG, "Reminder %d task option emergency threshold: %d, priority: %d",
                            all[i].Reminder_ID, option_threshold, option_priority);
                }

            } else {
                ESP_LOGW(TAG, "Task id %d for reminder %d not found", all[i].Task_ID, all[i].Reminder_ID);
            }

            double diff_seconds = difftime(now, all[i].Time_Created);
            uint32_t diff_days = diff_seconds / 86400;
            bool is_emergency = diff_days >= (uint32_t)option_threshold;
            ESP_LOGI(TAG, "Reminder %d: diff_days=%d, is_emergency=%s",
                    (int)all[i].Reminder_ID, (int)diff_days, is_emergency ? "true" : "false");

            if (active_in_timeslot || is_emergency) {
                uint8_t prio = is_emergency ? EMERGENCY_PRIORITY : (uint8_t)option_priority;
                if (prio > current_best_priority) {
                    current_best_priority = prio;
                    if(active_reminder != NULL) {
                        *active_reminder = all[i];
                    }
                    found = true;
                    ESP_LOGI(TAG, "Active reminder found: ID %d with priority %d",
                            all[i].Reminder_ID, prio);
                }
            }
        }
    }
    if(task != NULL) free(task);
    free(all);
    if(found && active_priority) {
        *active_priority = current_best_priority;
        ESP_LOGI(TAG, "Highest active priority set to %d", current_best_priority);
    }
    return found;
}

/**
 * Executes configured actions for an active reminder.
 *
 * Sets the LED (using default values from configuration),
 * sends a chirp, and displays a reminder message for 30 seconds.
 *
 * @param priority     The active reminder's priority (from task option; 1 is highest).
 * @param reminder_msg The message to display.
 */
static void execute_priority_action(uint8_t priority, const char *reminder_msg_line1,const char *reminder_msg_line2,const char *reminder_msg_line3,const char *reminder_msg_line4)
{
    ESP_LOGI(TAG, "Executing action for priority %d", priority);
    const priority_config_t *configs = get_priority_configs();
    uint8_t idx = (priority > 0 && priority < 4) ? priority - 1 : 2;
    ESP_LOGI(TAG, "Action config: LED[%d, %d, %d], Chirp ID %d",
             (int)configs[idx].default_led_red, (int)configs[idx].default_led_green,
             (int)configs[idx].default_led_blue, (int)configs[idx].default_chirp_id);
    set_led(configs[idx].default_led_red, configs[idx].default_led_green, configs[idx].default_led_blue);
    SEND_CHIRP(my_chirpQueue, configs[idx].default_chirp_id);

    // Display the reminder message for 30 seconds if the display is available.
    if(xSemaphoreTake(my_display_mutex, pdMS_TO_TICKS(1000))) {
        display_message(my_u8g2_ptr, get_wifi_status(), get_time_validity(),
                        reminder_msg_line1, reminder_msg_line2, reminder_msg_line3, reminder_msg_line4, 1);
        vTaskDelay(pdMS_TO_TICKS(30000));
        xSemaphoreGive(my_display_mutex);
    }


    ESP_LOGI(TAG, "Action execution completed for reminder: %s, Task: %s", reminder_msg_line1, reminder_msg_line2);
}

/**
 * Alarm Execution Task.
 *
 * Continuously checks reminders and executes the appropriate actions when reminders are active.
 * If the current time falls within timetable 0 (Do Not Disturb timeslots), execution is skipped.
 *
 * @param params Not used.
 */
void alarm_execution_task(void *params)
{
    ESP_LOGI(TAG, "Alarm execution task started");
    while(1)
    {
        //disable led
        set_led(0, 0, 0);
        vTaskDelay(pdMS_TO_TICKS(100));
        // Check if current time is within Do Not Disturb period (timetable 0)
        timetable_t dnd_timetable;
        if(load_timetable(0, &dnd_timetable) && check_timeslot_active(&dnd_timetable)) {
            ESP_LOGI(TAG, "Do Not Disturb period active. Skipping reminder execution.");
            vTaskDelay(pdMS_TO_TICKS(60000)); // Delay for 1 minute before rechecking
            continue;
        }

        uint8_t active_priority = 0;
        type1_reminder_t active_reminder;
        if(check_active_reminders(&active_priority, &active_reminder)) {
            ESP_LOGI(TAG, "Active reminder detected, ID: %d, Priority: %d",
                     active_reminder.Reminder_ID, active_priority);
            char msg_line1[64];
            char msg_line2[64];
            char msg_line3[64];
            char msg_line4[64];
            //active reminder id
            snprintf(msg_line1, sizeof(msg_line1), "Reminder %d active", active_reminder.Reminder_ID);
            //reminder task name
            task_t *task = get_task_by_id(active_reminder.Task_ID);
            if(task != NULL) {
                snprintf(msg_line2, sizeof(msg_line2), "T: %s", task->Name);
            } else {
                snprintf(msg_line2, sizeof(msg_line2), "Task ID: %d", active_reminder.Task_ID);
            }
            //reminder task option text
            if(task != NULL && active_reminder.Task_Option_Selected >= 0 &&
               active_reminder.Task_Option_Selected < TASK_MAX_OPTIONS) {
                snprintf(msg_line3, sizeof(msg_line3), "O: %s",
                         task->Options[active_reminder.Task_Option_Selected].display_text);
            } else {
                snprintf(msg_line3, sizeof(msg_line3), "Option: %d", active_reminder.Task_Option_Selected);
            }
            //aditional options number
            sniprintf(msg_line4, sizeof(msg_line4), "Add. Options: %d", active_reminder.Task_Additional_Option_Selected);
            if(task != NULL) free(task);
            execute_priority_action(active_priority, msg_line1,msg_line2,msg_line3,msg_line4);

            const priority_config_t *configs = get_priority_configs();
            uint8_t idx = (active_priority > 0 && active_priority < 4) ? active_priority - 1 : 2;
            ESP_LOGI(TAG, "Waiting for %d minutes after executing reminder", (int)configs[idx].display_interval_minutes);
            vTaskDelay(pdMS_TO_TICKS(configs[idx].display_interval_minutes * 60000));
        } else {
            ESP_LOGI(TAG, "No active reminders found, waiting 10 seconds");
            vTaskDelay(pdMS_TO_TICKS(10000));
        }
    }
}

/**
 * Initialize the alarm execution functionality.
 *
 * This function saves the provided pointers and creates the task responsible for
 * monitoring reminders and executing actions.
 *
 * @param chirp_q      Queue for chirp commands.
 * @param display_mux  Semaphore for display access.
 * @param u8g2_ptr_in  Pointer to the u8g2 display object.
 * @return ESP_OK if the task is successfully created, ESP_FAIL otherwise.
 */
esp_err_t alarm_execution_init(QueueHandle_t chirp_q,
                               SemaphoreHandle_t display_mux,
                               u8g2_t *u8g2_ptr_in)
{
    ESP_LOGI(TAG, "Initializing alarm execution module");
    my_chirpQueue    = chirp_q;
    my_display_mutex = display_mux;
    my_u8g2_ptr      = u8g2_ptr_in;

    if(xTaskCreate(alarm_execution_task, "alarm_execution_task", 4096, NULL, 1, NULL) == pdPASS) {
        ESP_LOGI(TAG, "Alarm execution task created successfully");
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Failed to create alarm execution task");
        return ESP_FAIL;
    }
}