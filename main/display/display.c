#include "display.h"

static const char *TAG = "display";


void task_test_SSD1306i2c(void* ignore) {
  u8g2_esp32_hal_t u8g2_esp32_hal = U8G2_ESP32_HAL_DEFAULT;
  u8g2_esp32_hal.bus.i2c.sda = PIN_SDA;
  u8g2_esp32_hal.bus.i2c.scl = PIN_SCL;
  u8g2_esp32_hal_init(u8g2_esp32_hal);

  u8g2_t u8g2;  // a structure which will contain all the data for one display
  u8g2_Setup_sh1106_i2c_128x64_noname_f(
      &u8g2, U8G2_R0,
      // u8x8_byte_sw_i2c,
      u8g2_esp32_i2c_byte_cb,
      u8g2_esp32_gpio_and_delay_cb);  // init u8g2 structure
  u8x8_SetI2CAddress(&u8g2.u8x8, 0x78);

  ESP_LOGI(TAG, "u8g2_InitDisplay");
  u8g2_InitDisplay(&u8g2);  // send init sequence to the display, display is in
                            // sleep mode after this,

  ESP_LOGI(TAG, "u8g2_SetPowerSave");
  u8g2_SetPowerSave(&u8g2, 0);  // wake up display
  ESP_LOGI(TAG, "u8g2_ClearBuffer");
  u8g2_ClearBuffer(&u8g2);
  ESP_LOGI(TAG, "u8g2_DrawBox");
  u8g2_DrawBox(&u8g2, 0, 26, 80, 6);
  u8g2_DrawFrame(&u8g2, 0, 26, 100, 6);

  ESP_LOGI(TAG, "u8g2_SetFont");
  u8g2_SetFont(&u8g2, u8g2_font_ncenB14_tr);
  ESP_LOGI(TAG, "u8g2_DrawStr");
  u8g2_DrawStr(&u8g2, 2, 17, "Hi nkolban!");
  ESP_LOGI(TAG, "u8g2_SendBuffer");
  u8g2_SendBuffer(&u8g2);

  ESP_LOGI(TAG, "All done!");

u8g2_ClearBuffer(&u8g2);
u8g2_SetBitmapMode(&u8g2, 1);
u8g2_SetFontMode(&u8g2, 1);
u8g2_SetFont(&u8g2, u8g2_font_timR24_tr);
u8g2_DrawStr(&u8g2, 1, 43, "18 30");
u8g2_SetFont(&u8g2, u8g2_font_timR18_tr);
u8g2_DrawStr(&u8g2, 78, 42, ":15");
u8g2_DrawFrame(&u8g2, 36, 37, 2, 2);
u8g2_DrawFrame(&u8g2, 36, 29, 2, 2);
u8g2_SetFont(&u8g2, u8g2_font_timR10_tr);
u8g2_DrawStr(&u8g2, 4, 60, "wendesday  11.12");
u8g2_SetDrawColor(&u8g2, 2);
u8g2_DrawLine(&u8g2, 0, 0, 0, 0);
u8g2_SetDrawColor(&u8g2, 1);
u8g2_DrawLine(&u8g2, 0, 46, 116, 46);
u8g2_SendBuffer(&u8g2);
  vTaskDelete(NULL);
}

/**
 * @brief Initialize a SSD1306 I2C display using a provided u8g2 object.
 *
 * This function configures the u8g2 object for a 128x64 SSD1306 display. The function
 * initializes the display hardware using your defined I2C pins (PIN_SDA and PIN_SCL) and
 * sets the power-save mode off. The configured u8g2 object can then be used for drawing.
 *
 * @param u8g2 Pointer to an unconfigured u8g2_t structure.
 */
void init_ssd1306_display(u8g2_t *u8g2)
{
    u8g2_esp32_hal_t u8g2_esp32_hal = U8G2_ESP32_HAL_DEFAULT;
    u8g2_esp32_hal.bus.i2c.sda = PIN_SDA;
    u8g2_esp32_hal.bus.i2c.scl = PIN_SCL;
    u8g2_esp32_hal_init(u8g2_esp32_hal);

    u8g2_Setup_sh1106_i2c_128x64_noname_f(
        u8g2,
        U8G2_R0,
        u8g2_esp32_i2c_byte_cb,
        u8g2_esp32_gpio_and_delay_cb);

    u8x8_SetI2CAddress(&u8g2->u8x8, 0x78);

    ESP_LOGI(TAG, "u8g2_InitDisplay");
    u8g2_InitDisplay(u8g2);
    ESP_LOGI(TAG, "u8g2_SetPowerSave");
    u8g2_SetPowerSave(u8g2, 0);
}

/**
 * @brief Display a task of type 1 on the screen.
 *
 * This function checks if the task type equals 1 and then displays the four option display texts.
 * It also shows the WiFi and time statuses (0 for "OK", any other value for "XX").
 * If an option's display text length exceeds 16 characters, an alternate font is used.
 *
 * @param wifi_status WiFi status: 0 means "OK", otherwise "XX".
 * @param time_status Time status: 0 means "OK", otherwise "XX".
 * @param task Pointer to the task structure to display.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t display_task_type1(uint8_t wifi_status, uint8_t time_status, const task_t *task, u8g2_t u8g2)
{
    if (task == NULL || task->Type != 1) {
        ESP_LOGE(TAG, "Invalid task or task type not equal 1");
        return ESP_ERR_INVALID_ARG;
    }

    // Begin drawing on the display.
    u8g2_ClearBuffer(&u8g2);
    u8g2_SetBitmapMode(&u8g2, 1);
    u8g2_SetFontMode(&u8g2, 1);
    u8g2_DrawBox(&u8g2, 0, 0, 127, 9);
    u8g2_DrawLine(&u8g2, 0, 22, 127, 22);
    u8g2_DrawLine(&u8g2, 0, 36, 127, 36);
    u8g2_DrawLine(&u8g2, 0, 50, 127, 50);

    // For each of the 4 options: choose font based on text length and draw the display text.
    for (int i = 0; i < TASK_MAX_OPTIONS; i++) {
        const char *disp_text = task->Options[i].display_text;
        if (strlen(disp_text) > 16) {
            u8g2_SetFont(&u8g2, u8g2_font_haxrcorp4089_tr);
        } else {
            u8g2_SetFont(&u8g2, u8g2_font_profont15_tr);
        }
        int y = 0;
        switch (i) {
            case 0: y = 20; break;
            case 1: y = 34; break;
            case 2: y = 48; break;
            case 3: y = 62; break;
            default: y = 20; break;
        }
        u8g2_DrawStr(&u8g2, 0, y, disp_text);
    }

    // Display WiFi and Time status.
    u8g2_SetDrawColor(&u8g2, 2);
    u8g2_SetFont(&u8g2, u8g2_font_5x7_tr);

    if(wifi_status == 0) {
        u8g2_DrawStr(&u8g2, 1, 7, "wifi OK");
    } else {
        u8g2_DrawStr(&u8g2, 1, 7, "wifi XX");
    }

    u8g2_DrawLine(&u8g2, 38, 0, 38, 8);

    if(time_status == 0) {
        u8g2_DrawStr(&u8g2, 40, 7, "time OK");
    } else {
        u8g2_DrawStr(&u8g2, 40, 7, "time XX");
    }

    u8g2_SendBuffer(&u8g2);

    return ESP_OK;
}