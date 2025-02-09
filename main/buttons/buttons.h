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


// Data type for button control commands (unchanged)
typedef struct {
    uint8_t button_id;
    uint8_t command; // 1: short press, 2: long press
} button_control_t;


void init_buttons(QueueHandle_t buttonControlQueue);


//static void install_buttons_isr(uint32_t button_gpio);

#endif // BUTTONS_H