#include "json_parser.h"

/**
 * @brief Fill a type1_reminder_t from a given task_t (Type=1).
 *
 * This function initializes a reminder structure with relevant data
 * from a task. The reminder ID is set to 0 so store_type1_reminder
 * can auto-assign a new ID. time(NULL) is used for Time_Created by default.
 *
 * @param task Pointer to a parsed task_t which should be of type 1.
 * @param reminder Pointer to the type1_reminder_t to be filled.
 * @param selected_option Chosen task option ID
 * @param additional_option Another optional selection
 */
void fill_type1_reminder_from_task(const task_t *task,
                                   type1_reminder_t *reminder,
                                   uint8_t selected_option,
                                   uint8_t additional_option)
{
    if (!task || !reminder) {
        return;
    }

    // In this design, it's assumed task->Type == 1
    reminder->Reminder_Type = 1;                       // Always 1
    reminder->Reminder_ID = 0;                         // Let store_type1_reminder assign
    reminder->Task_ID = (uint8_t)task->ID;             // Use the task's ID
    reminder->Task_Type = (uint8_t)task->Type;         // Same as the task's Type
    reminder->Task_Option_Selected = selected_option;   // Based on user input or logic
    reminder->Task_Additional_Option_Selected = additional_option;
    reminder->Time_Created = time(NULL);               // Current timestamp
    reminder->Time_Snoozed = 0;                        // Defaults to 0
}