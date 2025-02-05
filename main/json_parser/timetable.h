#ifndef TIMETABLE_H
#define TIMETABLE_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_TIMESLOTS 8
#define MAX_NAME_LEN 32

typedef struct {
    uint16_t Start_time;  // values 0–2400
    uint16_t End_time;
} timeslot_t;

typedef struct {
    uint8_t Type;  // less than 256
    char Name[MAX_NAME_LEN + 1]; // plus null terminator
    uint8_t ID;
    uint8_t times_count;  // number of active times in Times_active
    timeslot_t Times_active[MAX_TIMESLOTS];
} timetable_t;

void set_default_timetables(void);
char *retrieve_timetable_json(int id);
void log_timetable(int id);

esp_err_t store_timetable_json(const char *json_str, bool override_timetable);

bool parse_timetable_json(const char *json_str, timetable_t *timetable);

char *timetable_to_json(const timetable_t *timetable);


#endif // TIMETABLE_H