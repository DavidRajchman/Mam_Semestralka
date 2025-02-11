#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "ulp_lp_core.h"


#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "ulp_main.h"
#include "display.h"
#include "rfid.h"
#include "json_parser.h"
#include "nvs_config.h"
#include "wifi_time.h"
#include "buzzer.h"
#include "led.h"
#include "buttons.h"
#include "alarm_execution.h"

#define INTERUPT_PIN_LP 0
#define BUTTON1_PIN_LP 4
#define BUTTON2_PIN_LP 5
#define BUTTON3_PIN_LP 6
#define BUTTON4_PIN_LP 7

#define WAIT_FOR_USER_INPUT_TIMEOUT_MS 20000

static const char* wifi_credentials[2] = {
    "IoTtest",
    "2345678901"
};

uint8_t button_values[4] = {0};

extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[]   asm("_binary_ulp_main_bin_end");

//log tag
static const char *TAG = "main";
QueueHandle_t interruptQueue;

QueueHandle_t buttonControlQueue; //queue for button control commands
QueueHandle_t chirpQueue; //queue for chirp commands - based on play_chirp function
uint8_t button_control_active = 0; //flag for button - commands will be sent into the queue, only if this flag is set
//DISPLAY MUTEX
SemaphoreHandle_t display_mutex = NULL;

TaskHandle_t wifiTimeSyncTaskHandle = NULL;
TaskHandle_t rfid_tag_recieved_task_handle = NULL;
uint wifi_fail_counter = 0;
uint wifi_delay_cycles = 0; //delay cycles for wifi connection if previous connection failed - 1 cycle per update task

u8g2_t u8g2;
u8g2_t *u8g2_ptr = NULL;



void LP_gpio_inicializace(uint8_t gpio_num)
{
    rtc_gpio_init(gpio_num);
    rtc_gpio_set_direction(gpio_num, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pulldown_dis(gpio_num);
    rtc_gpio_pullup_en(gpio_num);
}
void init_ulp_program_and_gpio(void)
{
    rtc_gpio_init(INTERUPT_PIN_LP);
    rtc_gpio_set_direction(INTERUPT_PIN_LP, RTC_GPIO_MODE_OUTPUT_ONLY);
    rtc_gpio_pulldown_dis(INTERUPT_PIN_LP);
    rtc_gpio_pullup_dis(INTERUPT_PIN_LP);

    LP_gpio_inicializace(BUTTON1_PIN_LP);
    LP_gpio_inicializace(BUTTON2_PIN_LP);
    LP_gpio_inicializace(BUTTON3_PIN_LP);
    LP_gpio_inicializace(BUTTON4_PIN_LP);


    esp_err_t err = ulp_lp_core_load_binary(ulp_main_bin_start, (ulp_main_bin_end - ulp_main_bin_start));
    ESP_ERROR_CHECK(err);

    /* Start the program */
    ulp_lp_core_cfg_t cfg = {
        .wakeup_source = ULP_LP_CORE_WAKEUP_SOURCE_LP_TIMER,
        .lp_timer_sleep_duration_us = 5000,
    };

    err = ulp_lp_core_run(&cfg);
    ESP_ERROR_CHECK(err);

}


/**
 * @brief Task created when a PICC is detected.
 *
 * This task retrieves the task associated with the given RFID key from NVS,
 * parses the task JSON into a task_t structure, and displays it. THen it listens for user input.
 *
 * The RFID key is expected to be the key in NVS where the task JSON is stored.
 *
 * @param param Pointer to a string containing the RFID key.
 */
void task_RFID_tag_recieved(void *param)
{
    char *rfid_key = (char *)param;
    int reminder_id = 0;
    bool exit_task = false;
    int task_id = get_task_id_by_rfid(rfid_key);
    if (task_id < 0)
    {
        ESP_LOGE(TAG, "No task ID found for RFID key");
        vTaskDelete(NULL);
        return;
    }
    char *task_json = retrieve_task_json(task_id);
    free(rfid_key);  // free the copy made in the event handler

    if (task_json != NULL)
    {
        task_t task_buffer;
        type1_reminder_t reminder_buffer;
        if (!parse_task_json(task_json, &task_buffer))
        {
            ESP_LOGE(TAG, "Failed to parse task JSON for RFID key");
        }
        else
        {
            ESP_LOGI(TAG, "RFID task parsed successfully - loading display");
            if (task_buffer.Type == 1)
            {
                //take mutex
                if(xSemaphoreTake(display_mutex, 2000/portTICK_PERIOD_MS)){
                    uint8_t wifi_status = get_wifi_status();
                    uint8_t time_status = get_time_validity();
                    // Assuming wifi_status and time_status as 0 for "OK"
                    if(u8g2_ptr != NULL) {
                        display_task_type1(wifi_status, time_status, &task_buffer, *u8g2_ptr);
                    } else {
                        ESP_LOGE(TAG, "Display is not initialized");
                    }
                    // Wait for user input
                    //clear button queue
                    button_control_t button_control;
                    while (xQueueReceive(buttonControlQueue, &button_control, 0) == pdTRUE)
                    {
                        ESP_LOGI(TAG, "Cleared button control queue");
                    }
                    //set button control active
                    button_control_active = 1;
                    //wait for button press until timeout
                    TickType_t xTicksToWait = pdMS_TO_TICKS(WAIT_FOR_USER_INPUT_TIMEOUT_MS);
                    if (xQueueReceive(buttonControlQueue, &button_control, xTicksToWait) == pdTRUE)
                    {
                        ESP_LOGI(TAG, "Button %d pressed with command %d", button_control.button_id, button_control.command);
                        wifi_status = get_wifi_status();
                        time_status = get_time_validity();
                        switch (button_control.command) {
                            case 1:
                                ESP_LOGI(TAG, "Button %d short press while in task display mode", button_control.button_id);
                                //set reminder based on option = button_id
                                fill_type1_reminder_from_task(&task_buffer, &reminder_buffer, button_control.button_id, 0);
                                reminder_id = store_type1_reminder(&reminder_buffer, 0);
                                if (reminder_id > 0)
                                {
                                    ESP_LOGI(TAG, "Reminder stored with option %d successfully with ID %d",reminder_buffer.Task_Option_Selected, reminder_id);
                                    SEND_CHIRP(chirpQueue, 1);
                                    display_message(u8g2_ptr,get_wifi_status(),get_time_validity(),"","reminder stored", "succesfully","",1);
                                    vTaskDelay(2000 / portTICK_PERIOD_MS);
                                    //get_reminder_by_id(reminder_id, &reminder_buffer);
                                    //log_type1_reminder(&reminder_buffer);
                                }
                                else
                                {
                                    ESP_LOGE(TAG, "Failed to store reminder");
                                    //get wifi status and time status

                                    //display message
                                    if (reminder_id == -1)
                                    {
                                        display_message(u8g2_ptr, wifi_status, time_status, "Reminder exists", "long press but1", "to add another", "", 1);
                                        // Wait for user input
                                        if (xQueueReceive(buttonControlQueue, &button_control, xTicksToWait) == pdTRUE)
                                        {
                                            if (button_control.command == 2 && button_control.button_id == 0)
                                            {
                                                ESP_LOGI(TAG, "Reminder adition overide, adding reminder");

                                                //overides and stores reminder
                                                reminder_id = store_type1_reminder(&reminder_buffer, 1);
                                                if (reminder_id > 0)
                                                {
                                                    ESP_LOGI(TAG, "Reminder stored with option %d successfully with ID %d",reminder_buffer.Task_Option_Selected, reminder_id);
                                                    SEND_CHIRP(chirpQueue, 1);
                                                    //get_reminder_by_id(reminder_id, &reminder_buffer);
                                                    //log_type1_reminder(&reminder_buffer);
                                                    display_message(u8g2_ptr,get_wifi_status(),get_time_validity(),"","reminder stored", "succesfully","",1);
                                                    vTaskDelay(2000 / portTICK_PERIOD_MS);
                                                }
                                                else
                                                {
                                                    ESP_LOGE(TAG, "Failed to store reminder");
                                                    display_message(u8g2_ptr, wifi_status, time_status, "Failed to store", "reminder", "", "", 1);
                                                    vTaskDelay(2000 / portTICK_PERIOD_MS);
                                                }
                                            }
                                            else if (button_control.command == 100)
                                            {
                                                ESP_LOGI(TAG, "TASK SHUTDOWN REQUESTED, exiting");
                                            }



                                        }
                                    }
                                    else{
                                    display_message(u8g2_ptr, wifi_status, time_status, "Failed to store", "reminder", "", "", 1);
                                    vTaskDelay(2000 / portTICK_PERIOD_MS);
                                    }
                                }

                                break;
                            case 2:
                                ESP_LOGI(TAG, "Button %d long press while in task display mode" , button_control.button_id);
                                uint8_t additional_option = 0;
                                display_message(u8g2_ptr, wifi_status, time_status,"Reminder options:", "Press button X", "to set reminder",  "with value of butt.X", 1);
                                //LONG PRESS MEANS ADITIONAL OPTIONS MENU - SHOW ADITIONAL OPTIONS MENU
                                if (xQueueReceive(buttonControlQueue, &button_control, xTicksToWait) == pdTRUE)
                                {
                                    if (button_control.command == 1)
                                    {
                                        ESP_LOGI(TAG, "Button %d short press while in task aditional options mode", button_control.button_id);
                                        switch (button_control.button_id)
                                    {
                                        case 0:
                                            additional_option = 1;
                                            break;
                                        case 1:
                                            additional_option = 2;
                                            break;
                                        case 2:
                                            additional_option = 3;
                                            break;
                                        case 3:
                                            additional_option = 4;
                                            break;
                                        default:
                                            additional_option = 0;
                                            break;
                                    }
                                    }
                                    else
                                    {
                                        ESP_LOGI(TAG, "Undesired button command %d exiting", button_control.command);
                                        exit_task = true;
                                    }



                                }

                                if (!exit_task)
                                {

                                    //FROM HERE SAME AS CASE 1 except additional_option is set
                                    //set reminder based on option = button_id
                                    fill_type1_reminder_from_task(&task_buffer, &reminder_buffer, button_control.button_id, additional_option);
                                    reminder_id = store_type1_reminder(&reminder_buffer, 0);
                                    if (reminder_id > 0)
                                    {
                                        ESP_LOGI(TAG, "Reminder stored with option %d, additonal option %d | successfully with ID %d",reminder_buffer.Task_Option_Selected,reminder_buffer.Task_Additional_Option_Selected, reminder_id);
                                        SEND_CHIRP(chirpQueue, 1);
                                        display_message(u8g2_ptr,get_wifi_status(),get_time_validity(),"","reminder stored", "succesfully","",1);
                                        vTaskDelay(2000 / portTICK_PERIOD_MS);

                                        //get_reminder_by_id(reminder_id, &reminder_buffer);
                                        //log_type1_reminder(&reminder_buffer);
                                    }
                                    else
                                    {
                                        ESP_LOGE(TAG, "Failed to store reminder");
                                        //get wifi status and time status
                                        wifi_status = get_wifi_status();
                                        time_status = get_time_validity();
                                        //display message
                                        if (reminder_id == -1)
                                        {
                                            display_message(u8g2_ptr, wifi_status, time_status, "Reminder exists", "long press but1", "to add another", "", 1);
                                            // Wait for user input
                                            if (xQueueReceive(buttonControlQueue, &button_control, xTicksToWait) == pdTRUE)
                                            {
                                                if (button_control.command == 2 && button_control.button_id == 1)
                                                {
                                                    ESP_LOGI(TAG, "Reminder adition overide, adding reminder");

                                                    //overides and stores reminder
                                                    reminder_id = store_type1_reminder(&reminder_buffer, 1);
                                                    if (reminder_id > 0)
                                                    {
                                                        ESP_LOGI(TAG, "Reminder stored with option %d successfully with ID %d",reminder_buffer.Task_Option_Selected, reminder_id);
                                                        SEND_CHIRP(chirpQueue, 1);
                                                        display_message(u8g2_ptr,get_wifi_status(),get_time_validity(),"","reminder stored", "succesfully","",1);
                                                        vTaskDelay(2000 / portTICK_PERIOD_MS);
                                                        //get_reminder_by_id(reminder_id, &reminder_buffer);
                                                        //log_type1_reminder(&reminder_buffer);
                                                    }
                                                    else
                                                    {
                                                        ESP_LOGE(TAG, "Failed to store reminder");
                                                        display_message(u8g2_ptr, wifi_status, time_status, "Failed to store", "reminder", "", "", 1);
                                                        vTaskDelay(2000 / portTICK_PERIOD_MS);
                                                    }
                                                }
                                                else if (button_control.command == 100)
                                                {
                                                    ESP_LOGI(TAG, "TASK SHUTDOWN REQUESTED, exiting");
                                                }



                                            }
                                        }
                                        else{
                                        display_message(u8g2_ptr, wifi_status, time_status, "Failed to store", "reminder", "", "", 1);
                                        vTaskDelay(2000 / portTICK_PERIOD_MS);
                                        }
                                    }
                                }
                                break;
                            case 100:
                                ESP_LOGI(TAG, "TASK SHUTDOWN REQUESTED, exiting");
                                break;
                            default:
                                ESP_LOGI(TAG, "Unknown button command %d", button_control.command);
                                break;
                        }
                    }
                    else
                    {
                        ESP_LOGI(TAG, "No button press detected during task display mode == timeout");
                    }

                    //give mutex
                    xSemaphoreGive(display_mutex);
                    button_control_active = 0;
                }
                else
                {
                    ESP_LOGW(TAG, "RFID_TASK-Failed to take display mutex");
                }


            }
            else
            {
                ESP_LOGI(TAG, "Task is not type 1, not displayed");
            }
        }
        free(task_json);
    }
    else
    {
        ESP_LOGE(TAG, "No task JSON retrieved for RFID key");
    }
    ESP_LOGI(TAG, "RFID task task_RFID_tag_recieved finished");
    rfid_tag_recieved_task_handle = NULL;
    vTaskDelete(NULL);
}



/**
 * @brief Sends a shutdown command to the button control queue
 *
 * This function creates a button control message with ID 0 and command 100,
 * which signals a shutdown command, and sends it to the buttonControlQueue.
 * Used when a new RFID tag is detected while a task is already running.
 * It will wait until the task is finished
 */
void shutdown_rfid_tag_recieved_task(TaskHandle_t task)
{
    if (task == NULL)
    {
        ESP_LOGE(TAG, "Task handle is NULL");
        return;
    }
    button_control_t button_control;
    button_control.button_id = 0;
    button_control.command = 100;
    xQueueSend(buttonControlQueue, &button_control, 0);
    uint8_t timeout = 0;

    eTaskState state = eTaskGetState(task);
    //while true loop waiting for task state to be deleted
    while (state != eDeleted && state != eReady && task != NULL && timeout < 200)
    {
        vTaskDelay(10 / portTICK_PERIOD_MS);
        //log task state
        state = eTaskGetState(task);
        ESP_LOGI(TAG, "Task state: %d", state);
        timeout++;
    }
    ESP_LOGI(TAG, "Task has been deleted");
    task = NULL;
}

void on_RFID_state_changed(void *arg, esp_event_base_t base, int32_t event_id, void *data)
{
    rc522_picc_state_changed_event_t *event = (rc522_picc_state_changed_event_t *)data;
    rc522_picc_t *picc = event->picc;

    if (picc->state == RC522_PICC_STATE_ACTIVE) {
        rc522_picc_print(picc);


        char buffer[RC522_PICC_UID_STR_BUFFER_SIZE_MAX];
        //rc522_picc_uid_to_str(&picc->uid,buffer,sizeof(buffer));
        uid_to_str_no_space(&picc->uid,buffer,sizeof(buffer));
        //ESP_LOGI(TAG, "UID no space = |%s|",buffer);
        if (buffer[0] == '\0')
        {
            ESP_LOGE(TAG, "Failed to convert UID to string");
            return;
        }
        //use buffer as a key for the task directly
        char *rfid_key = strdup(buffer);
        if (rfid_key == NULL)
        {
            ESP_LOGE(TAG, "Failed to allocate memory for RFID key");
            return;
        }
        SEND_CHIRP(chirpQueue, 5);
        //if there is a task already running, delete it
        if (rfid_tag_recieved_task_handle != NULL)
        {
            ESP_LOGI(TAG, "Task already running - shutting down");
            shutdown_rfid_tag_recieved_task(rfid_tag_recieved_task_handle);
        }
        vTaskDelay(20 / portTICK_PERIOD_MS);
        xTaskCreate(task_RFID_tag_recieved, "task_RFID_tag_recieved", 4096, rfid_key, 1, &rfid_tag_recieved_task_handle);



        // v picc->uid.value je pole charu(8 bit uint), kazdy char odpovida 2 hex hodnotam -  extrakce nasledovnce:
        /*
        ESP_LOGI(TAG,"uid:|");
        for (uint32_t i = 0; i < picc->uid.length; i++){
            ESP_LOGI(TAG,"%X",picc->uid.value[i]);
        }
        ESP_LOGI(TAG,"|end of uid\n");
        */
    }
    else if (picc->state == RC522_PICC_STATE_IDLE && event->old_state >= RC522_PICC_STATE_ACTIVE) {
        ESP_LOGI(TAG, "Card has been removed");
    }
}

void task_update_tick(void *params)
{
    uint8_t wifi_status, time_status;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(1000); // period: 1 second

    while (1)
    {
        // Wait until the next cycle.
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        // Update the display
        wifi_status = get_wifi_status();
        time_status = get_time_validity();
        if(time_status == 1)
        {
            if (wifiTimeSyncTaskHandle != NULL) {
                eTaskState state = eTaskGetState(wifiTimeSyncTaskHandle);
                if (state != eDeleted) {
                    ESP_LOGD(TAG, "Time sync task is still running. State=%d", state);
                } else {
                    ESP_LOGD(TAG, "Time sync task has been deleted - creating.");
                    if(wifi_status == WIFI_STATUS_DISCONNECTED_FAIL && wifi_delay_cycles < 1)
                    {
                        wifi_delay_cycles = wifi_fail_counter*120;
                        wifi_fail_counter++;
                        xTaskCreate(wifi_sync_sntp_time_once_task, "wifi_sync_sntp_time_once_task", 8000, (void*)wifi_credentials, 1, &wifiTimeSyncTaskHandle);
                        if (wifi_delay_cycles > 90000)
                        {
                            wifi_delay_cycles = 90000;
                        }
                        ESP_LOGW(TAG, "Failed to connect to wifi - retrying in %d seconds", wifi_delay_cycles);

                    }
                    else if(wifi_status == WIFI_STATUS_DISCONNECTED_FAIL)
                    {
                        wifi_delay_cycles--;
                        ESP_LOGW(TAG, "Failed to connect to wifi - retrying in %d seconds", wifi_delay_cycles);

                    }
                    else
                    {
                        wifi_fail_counter = 0;
                        wifi_delay_cycles = 0;
                        xTaskCreate(wifi_sync_sntp_time_once_task, "wifi_sync_sntp_time_once_task", 8000, (void*)wifi_credentials, 1, &wifiTimeSyncTaskHandle);

                    }


                }
            } else {
                ESP_LOGI(TAG, "Time sync task handle is NULL; task not running - creating.");
                xTaskCreate(wifi_sync_sntp_time_once_task, "wifi_sync_sntp_time_once_task", 8000, (void*)wifi_credentials, 1, &wifiTimeSyncTaskHandle);
            }

        }
        //take mutex
       if(xSemaphoreTake(display_mutex, 10/portTICK_PERIOD_MS)){
            if (u8g2_ptr != NULL)
            {
                display_idle_clock_screen(&u8g2, wifi_status, time_status);
            }
            else
            {
                ESP_LOGE(TAG, "Display is not initialized");
            }
            //release mutex
            xSemaphoreGive(display_mutex);
        }
        else
        {
            ESP_LOGW(TAG, "UPDATE_TASK-Failed to take display mutex");
        }
    }
}

/*
   Task: task_show_running_reminders_menu
   - Displays a menu of running reminders using the proper reminder API.
   - Top menu (list mode): shows up to 4 reminders per page.
     Format per line: "<Reminder_ID>: <Option text>"
     The current selection is highlighted.
   - Detail view (bottom layer): displays details for the selected reminder:
       Line1: "Option: <Option text>"
       Line2: "Add. Opt: <Task_Additional_Option_Selected>"
       Line3: "Date: <DD,MM,YYYY>" (from Time_Created)
   - Navigation:
       Button 0: Up
       Button 1: Down
       Button 2: Confirm (enter detail view)
       Button 3: Back (exit detail view or exit the menu)
   - Uses the buttonControlQueue. Takes the display_mutex before drawing.
*/
void task_show_running_reminders_menu(void *params)
{
    char *TAG_RM = "reminder_menu";
    int current_index = 0; // Index into the reminders list
    bool detail_mode = false;
    const int items_per_page = 4;
    int page_start = 0;
    button_control_t btn;
    // Set the button control active flag to 1 to allow button control commands.
    button_control_active = 1;
    uint8_t wifi_status = 0, time_status = 0; // Example status values
    // Wait for the display mutex. and hold it for the duration of the task.
    if(xSemaphoreTake(display_mutex, 1000/portTICK_PERIOD_MS)) {

        while (1) {
            // Get the current number of reminders.
            size_t total = get_num_of_reminders();
            // Allocate an array to hold the reminders, if any.
            type1_reminder_t *reminders = NULL;
            if (total > 0) {
                reminders = malloc(total * sizeof(type1_reminder_t));
                if (reminders) {
                    total = get_all_type1_reminders(reminders, total);
                } else {
                    ESP_LOGE(TAG_RM, "Memory allocation failed for reminders");
                    total = 0;
                }
            }

            // Ensure current_index is within bounds.
            if (total == 0) {
                current_index = 0;
            } else if (current_index >= (int)total) {
                current_index = total - 1;
            }

            // Draw the menu.
            u8g2_ClearBuffer(u8g2_ptr);
            wifi_status = get_wifi_status();
            time_status = get_time_validity();
            render_top_info_bar(u8g2_ptr, wifi_status, time_status);

            if (!detail_mode) {
                // List mode: show up to 4 reminders per page.
                //if 0 reminders, display message
                if (total == 0) {
                    display_message(u8g2_ptr, wifi_status, time_status, "No reminders", "", "", "", 1);
                    u8g2_SendBuffer(u8g2_ptr);
                    // Free the reminders array.
                    if (reminders) {
                        free(reminders);
                        reminders = NULL;
                    }
                    vTaskDelay(2000 / portTICK_PERIOD_MS);
                    ESP_LOGI(TAG_RM, "Exiting reminders menu.");
                    button_control_active = 0;
                    xSemaphoreGive(display_mutex);
                    vTaskDelete(NULL);

                }
                page_start = (current_index / items_per_page) * items_per_page;
                for (int row = 0; row < items_per_page; row++) {
                    int idx = page_start + row;
                    char buf[40] = "";
                    int y;
                    if (idx < (int)total && reminders != NULL) {
                        task_t *task = get_task_by_id(reminders[idx].Task_ID);
                        const char *opt_text = "N/A";
                        if (task != NULL) {
                            opt_text = task->Options[reminders[idx].Task_Option_Selected].display_text;
                            free(task);
                        }
                        snprintf(buf, sizeof(buf), "%d: %s", reminders[idx].Reminder_ID, opt_text);
                    }
                    // Set vertical position.
                    switch (row) {
                        case 0: y = 22; break;
                        case 1: y = 35; break;
                        case 2: y = 47; break;
                        case 3: y = 59; break;
                        default: y = 22; break;
                    }
                    // Highlight current selection.
                    if (idx == current_index && idx < (int)total) {
                        u8g2_SetDrawColor(u8g2_ptr, 1);
                        u8g2_DrawBox(u8g2_ptr, 0, y - 9, u8g2_GetDisplayWidth(u8g2_ptr), 12);
                        u8g2_SetDrawColor(u8g2_ptr, 0);
                        u8g2_DrawStr(u8g2_ptr, 2, y, buf);
                        u8g2_SetDrawColor(u8g2_ptr, 1);
                    } else {
                        u8g2_DrawStr(u8g2_ptr, 2, y, buf);
                    }
                }
                u8g2_SendBuffer(u8g2_ptr);
            } else {
                // Detail mode: show details for the selected reminder.
                if (current_index < (int)total && reminders != NULL) {
                    type1_reminder_t *rem = &reminders[current_index];
                    task_t *task = get_task_by_id(rem->Task_ID);
                    const char *opt_text = "N/A";
                    if (task != NULL) {
                        opt_text = task->Options[rem->Task_Option_Selected].display_text;
                        free(task);
                    }
                    char line1[64], line2[64], line3[64], line4[64];
                    snprintf(line1, sizeof(line1), "%s", opt_text);
                    snprintf(line2, sizeof(line2), "Additional Opt: %d", rem->Task_Additional_Option_Selected);
                    char date_str[32];
                    {
                        struct tm *tm_info = localtime(&rem->Time_Created);
                        if (tm_info) {
                            snprintf(date_str, sizeof(date_str), "%02d,%02d,%04d", tm_info->tm_mday, tm_info->tm_mon + 1, tm_info->tm_year + 1900);
                        } else {
                            strncpy(date_str, "00,00,0000", sizeof(date_str));
                        }
                    }
                    snprintf(line3, sizeof(line3), "Date: %s", date_str);
                    //dispoly timeslot ids in line 4
                    if(task->Options[rem->Task_Option_Selected].timeslot_count > 0) {
                        strcpy(line4, "Slots: ");
                        char buf[10];
                        for (int i = 0; i < task->Options[rem->Task_Option_Selected].timeslot_count; i++) {
                            snprintf(buf, sizeof(buf), "%d ", task->Options[rem->Task_Option_Selected].timeslots[i]);
                            strcat(line4, buf);
                        }
                    } else {
                        strcpy(line4, "No timeslots");
                    }


                    // Truncate each line to a maximum of 23 characters.
                    if(strlen(line1) > 23) line1[23] = '\0';
                    if(strlen(line2) > 23) line2[23] = '\0';
                    if(strlen(line3) > 23) line3[23] = '\0';
                    if(strlen(line4) > 23) line4[23] = '\0';


                    display_message(u8g2_ptr, wifi_status, time_status, line1, line2, line3, line4, 1);
                }
            }





            // Wait for button input.
            if (xQueueReceive(buttonControlQueue, &btn, pdMS_TO_TICKS(250)) == pdTRUE) {
                if (!detail_mode) {
                    // List mode navigation.
                    switch (btn.button_id) {
                        case 0: // Confirm - enter detail view.
                            if (btn.command == 1){
                                detail_mode = true;
                            }
                            break;
                        case 1: // Up
                            if (current_index > 0)
                                current_index--;
                            break;
                        case 2: // Down
                            if (current_index < 0) current_index = 0;
                            if (current_index < (int)get_num_of_reminders() - 1)
                                current_index++;
                            break;
                        case 3: // Back - exit menu task.
                            ESP_LOGI(TAG_RM, "Exiting reminders menu.");
                            button_control_active = 0;
                            xSemaphoreGive(display_mutex);
                            vTaskDelete(NULL);
                            break;
                        default:
                            break;
                    }
                } else {
                    // Detail mode navigation.
                    // If button 0 long press, delete current reminder.
                    if (btn.button_id == 0 && btn.command == 2) {
                        esp_err_t del_err = delete_reminder(reminders[current_index].Reminder_ID);
                        if (del_err == ESP_OK) {
                            ESP_LOGI(TAG_RM, "Deleted reminder %d", reminders[current_index].Reminder_ID);
                            SEND_CHIRP(chirpQueue,2);
                            display_message(u8g2_ptr, wifi_status, time_status, "Reminder deleted", "", "", "", 1);
                            vTaskDelay(2000 / portTICK_PERIOD_MS);
                            if (current_index > 0) current_index--;
                        } else {
                            ESP_LOGE(TAG_RM, "Failed to delete reminder %d", reminders[current_index].Reminder_ID);
                            display_message(u8g2_ptr, wifi_status, time_status, "Delete failed", "", "", "", 1);
                            vTaskDelay(2000 / portTICK_PERIOD_MS);
                        }
                        detail_mode = false; // exit detail view after deletion
                    }
                    // In detail mode, Back returns to list mode.
                    else if (btn.button_id == 3) {
                        detail_mode = false;
                    }
                }
            }
            // Free the reminders array.
            if (reminders) {
                free(reminders);
                reminders = NULL;
            }
        }

    }
    else
    {
        ESP_LOGW(TAG_RM, "REMINDER_MENU_TASK-Failed to take display mutex");
    }
}

void app_main(void)
{
    //FOR TESTING
    //ERASE NVS PARTITION DATA
    //ESP_ERROR_CHECK(nvs_flash_erase_partition(NVS_PARTITION));



    interruptQueue = xQueueCreate(10, sizeof(int));
    buttonControlQueue = xQueueCreate(10, sizeof(button_control_t));
    display_mutex = xSemaphoreCreateMutex();
    chirpQueue = xQueueCreate(10, sizeof(uint8_t));
    u8g2_ptr = &u8g2;

    init_buttons(buttonControlQueue, task_show_running_reminders_menu, NULL, NULL, NULL);
    init_ulp_program_and_gpio();
    buzzer_init();
    init_led();
    nvs_flash_init(); //nvs for wifi
    wifi_init();// initializes wifi - does not connect
    esp_err_t err = nvs_flash_init_partition(NVS_PARTITION); //nvs for json data
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize custom NVS partition: %s", esp_err_to_name(err));
        return;
    }




    init_ssd1306_display(&u8g2);
    rfid_setup(on_RFID_state_changed);


    xTaskCreate(play_chirp_task, "play_chirp_task", 2048, (void *)chirpQueue, 3, NULL);
    xTaskCreate(wifi_sync_sntp_time_once_task, "wifi_sync_sntp_time_once_task", 8000, (void*)wifi_credentials, 1, &wifiTimeSyncTaskHandle);
    xTaskCreate(task_update_tick, "task_update_tick", 4096, NULL, 2, NULL);



    set_default_timetables();
    //log_timetable(1);
    //log_timetable(2);
    //log_timetable(3);
    //log_timetable(0);

    set_default_tasks();
    //log_task(1);
    //log_task(2);
    //log_task(3);

    //alarm execution init
    alarm_execution_init(chirpQueue,display_mutex,u8g2_ptr);
    uint8_t led_value = 255;
    while (false)
    {
        if (led_value > 100)
        {
            //led_value = 100;
        }
        set_led(led_value, led_value, led_value);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        set_led(0, 0, led_value);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        set_led(0, led_value, 0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        //SEND_CHIRP(chirpQueue, 4);
        set_led(led_value, 0, 0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        led_value = led_value-10;
        ESP_LOGI(TAG, "LED value: %d", led_value);
        //log current time
        time_t now;
        struct tm timeinfo;
        time(&now);
        localtime_r(&now, &timeinfo);
        ESP_LOGI(TAG, "Current time: %s", asctime(&timeinfo));


    }

}