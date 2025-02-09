#ifndef LED_H
#define LED_H

#include "led_strip.h"
#include "esp_err.h"
#include "esp_log.h"

#define LED_GPIO 8

void init_led();
void set_led(uint32_t red, uint32_t green, uint32_t blue);

#endif // LED_H