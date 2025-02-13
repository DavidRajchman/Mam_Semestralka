
#ifndef DISPLAY_H
#define DISPLAY_H

#define PIN_SDA 22
#define PIN_SCL 23

#include <u8g2.h>
#include <u8g2_esp32_hal.h>
#include "esp_log.h"
#include "json_parser.h"
#include <time.h>
#include <stdio.h>

void task_test_SSD1306i2c(void* ignore);
void init_ssd1306_display(u8g2_t *u8g2);
esp_err_t display_task_type1(uint8_t wifi_status, uint8_t time_status, const task_t *task, u8g2_t u8g2);
void display_idle_clock_screen(u8g2_t *u8g2, uint8_t wifi_status, uint8_t time_status);
void display_message(u8g2_t *u8g2, uint8_t wifi_status, uint8_t time_status, const char *line1, const char *line2,
                     const char *line3, const char *line4, int center);
void render_top_info_bar(u8g2_t *u8g2, uint8_t wifi_status, uint8_t time_status);



#endif