#ifndef JSON_PARSE_H
#define JSON_PARSE_H


#include "cJSON.h"
#include <time.h>
#include "esp_log.h"
#include "esp_err.h"
#include <string.h>
#include <stdbool.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "nvs_config.h" // NVS partition name

struct type1_reminder;
typedef struct type1_reminder type1_reminder_t;

//json structs definitions
#include "reminder.h"
#include "timetable.h"
#include "task.h"







//functions that use multiple objects (timetable, task, reminder)
void fill_type1_reminder_from_task(const task_t *task,
                                   type1_reminder_t *reminder,
                                   uint8_t selected_option,
                                   uint8_t additional_option);


#endif