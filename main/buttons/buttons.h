#ifndef BUTTONS_H
#define BUTTONS_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <stdint.h>
#include "ulp_lp_core.h"
#include "ulp_main.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"

#define BUTTONS_INTERUPT_PIN 1

// Standard FreeRTOS task function typedef for quick action tasks.
typedef void (*quick_action_task_func_t)(void *);

// Data type for button control commands (unchanged)
typedef struct {
    uint8_t button_id;
    uint8_t command; // 1: short press, 2: long press
} button_control_t;


void init_buttons(QueueHandle_t buttonControlQueue, quick_action_task_func_t task0,
                  quick_action_task_func_t task1, quick_action_task_func_t task2, quick_action_task_func_t task3);
void quick_action_task_launcher(int button_id);


//static void install_buttons_isr(uint32_t button_gpio);

#endif // BUTTONS_H