#ifndef ALARM_EXECUTION_H
#define ALARM_EXECUTION_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "u8g2.h"          // for u8g2_t
#include "json_parser.h"   // for timetable_t, type1_reminder_t, etc.
#include "led.h"
#include "buzzer.h"
#include "display.h"
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include "wifi_time.h"

#define EMERGENCY_PRIORITY 3 // Priority level for emergency reminders

/**
 * @brief Initialize the alarm execution functionality.
 *
 * This function creates the task that continuously monitors reminders and executes
 * the configured actions.
 *
 * @param chirp_q      QueueHandle_t for chirp commands.
 * @param display_mux  SemaphoreHandle_t for the display.
 * @param u8g2_ptr     Pointer to the u8g2 display object.
 * @return ESP_OK if the task is successfully created, ESP_FAIL otherwise.
 */
esp_err_t alarm_execution_init(QueueHandle_t chirp_q,
                               SemaphoreHandle_t display_mux,
                               u8g2_t *u8g2_ptr);

#endif // ALARM_EXECUTION_H