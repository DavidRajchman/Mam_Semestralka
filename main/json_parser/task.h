#ifndef TASK_H
#define TASK_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#define MAX_TASK_NAME_LEN         32
#define MAX_RFID_UID_LEN          17
#define MAX_OPTION_DISPLAY_LEN    32
#define TASK_MAX_OPTIONS          4
#define MAX_TASK_TIMESLOTS        8  // maximum number of timeslots per option

#define RFID_MAP_NVS_KEY "RFID_MAP" // NVS key for RFID UID-to-task ID mapping
#define RFID_MAP_NAMESPACE "mapping"  // NVS namespace for RFID mapping
#define MAX_RFID_MAPPINGS 16

typedef struct {
    char display_text[MAX_OPTION_DISPLAY_LEN + 1];    // plus null terminator
    uint8_t timeslot_count;                           // number of timeslots provided
    uint8_t timeslots[MAX_TASK_TIMESLOTS];            // each timeslot value (0-16)
    int8_t priority;                                  // priority value
    int8_t days_till_em;                              // days till event
} task_option_t;

typedef struct {
    uint8_t Type;
    char Name[MAX_TASK_NAME_LEN + 1];
    uint8_t ID;
    char RFID_UID[MAX_RFID_UID_LEN + 1];  // if not assigned, empty string
    task_option_t Options[TASK_MAX_OPTIONS];  // always exactly 4 options
} task_t;

typedef struct {
    char rfid_uid[17]; // 16 hex characters + null terminator
    uint8_t task_id;   // Task ID mapped to this RFID UID
} rfid_entry_t;

typedef struct {
    uint8_t count;                   // Number of valid entries in the array.
    rfid_entry_t entries[MAX_RFID_MAPPINGS];
} rfid_mapping_t;

// Parses a JSON string into a task_t structure.
// Returns true on success, false on error.
bool parse_task_json(const char *json_str, task_t *task);

// Converts a task_t structure to a JSON string.
// The returned string must be freed by the caller.
char *task_to_json(const task_t *task);

// Stores the task JSON string in NVS under namespace "Jtask".
// The key is either the RFID_UID if non-empty, or "T<ID>" if RFID_UID is empty.
// If a value already exists and override_task is false, it returns an error.
esp_err_t store_task_json(const char *json_str, bool override_task);

// Retrieves the task JSON string from NVS by ID (searching key "T<ID>").
// The returned string must be freed by the caller.
char *retrieve_task_json(int id);

int get_task_id_by_rfid(const char *rfid_uid);

task_t *get_task_by_id(int id);


esp_err_t assign_rfid_to_task(const char *rfid_uid, uint8_t task_id);


// Retrieves and logs a task (by ID) stored in NVS.
void log_task(int id);

// Creates default task(s) and stores them in NVS (without assigning any RFID_UID).
void set_default_tasks(void);

#endif // TASKS_H