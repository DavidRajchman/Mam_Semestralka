#ifndef REMINDER_H
#define REMINDER_H

#include "json_parser.h"
#include <stdint.h>
#include <time.h>
#include "esp_err.h"

/**
 * @brief Struct for a type1 reminder, stored as a fixed-size binary blob.
 */
typedef struct type1_reminder {
    uint8_t  Reminder_Type;  // For now always 1
    uint8_t  Reminder_ID;    // Assigned automatically, forms the NVS key ("R<ID>")
    uint8_t  Task_ID;
    uint8_t  Task_Type;
    uint8_t  Task_Option_Selected;
    uint8_t  Task_Additional_Option_Selected;
    time_t   Time_Created;
    time_t   Time_Snoozed;   // Defaults to 0 if unused
} type1_reminder_t;


int store_type1_reminder(const type1_reminder_t *reminder_in, uint8_t create_even_if_exists);



size_t get_num_of_reminders(void);


size_t get_all_type1_reminders(type1_reminder_t *reminders, size_t max_count);


esp_err_t update_type1_reminder(const type1_reminder_t *reminder);


esp_err_t delete_reminder(uint8_t reminder_id);

esp_err_t get_reminder_by_id(uint8_t reminder_id, type1_reminder_t *reminder);

void log_type1_reminder(const type1_reminder_t *reminder);



#endif // REMINDER_H