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
 * @brief Renders the top info bar showing WiFi and Time status
 *
 * @param u8g2 Pointer to the display context
 * @param wifi_status  0 for OK, non-zero otherwise
 * @param time_status  0 for OK, non-zero otherwise
 */
static void render_top_info_bar(u8g2_t *u8g2, uint8_t wifi_status, uint8_t time_status)
{
    u8g2_DrawBox(u8g2, 0, 0, 127, 9);

    u8g2_SetDrawColor(u8g2, 0);
    u8g2_SetFont(u8g2, u8g2_font_5x7_tr);

    switch (wifi_status)
    {
    case 0:
        u8g2_DrawStr(u8g2, 1, 7, "wifi CONN");
        break;
    case 1:
        u8g2_DrawStr(u8g2, 1, 7, "wifi DISC");
        break;


    default:
        u8g2_DrawStr(u8g2, 1, 7, "wifi FAIL");
        break;
    }

    u8g2_DrawLine(u8g2, 53, 0, 53, 8);

    if (time_status == 0) {
        u8g2_DrawStr(u8g2, 60, 7, "time SYNC");
    } else {
        u8g2_DrawStr(u8g2, 55, 7, "time UNSYNC");
    }
    //reset draw color
    u8g2_SetDrawColor(u8g2, 1);
}

/**
 * @brief Renders Task Type 1 data (4 options) along with a top bar
 *        showing WiFi and time status.
 *
 * @param wifi_status  0 for OK, non-zero otherwise
 * @param time_status  0 for OK, non-zero otherwise
 * @param task         Pointer to the task_t structure
 * @param u8g2         u8g2 display object
 * @return esp_err_t   ESP_ERR_INVALID_ARG if task is null or not type 1, else ESP_OK
 */
esp_err_t display_task_type1(uint8_t wifi_status, uint8_t time_status, const task_t *task, u8g2_t u8g2)
{
    if (task == NULL || task->Type != 1) {
        ESP_LOGE(TAG, "Invalid task or task type not equal to 1");
        return ESP_ERR_INVALID_ARG;
    }

    // Begin drawing on the display
    u8g2_ClearBuffer(&u8g2);
    u8g2_SetBitmapMode(&u8g2, 1);
    u8g2_SetFontMode(&u8g2, 1);

    // Draw lines
    u8g2_DrawLine(&u8g2, 0, 22, 127, 22);
    u8g2_DrawLine(&u8g2, 0, 36, 127, 36);
    u8g2_DrawLine(&u8g2, 0, 50, 127, 50);

    // Render the four options
    for (int i = 0; i < TASK_MAX_OPTIONS; i++) {
        const char *disp_text = task->Options[i].display_text;
        if (strlen(disp_text) > 16) {
            u8g2_SetFont(&u8g2, u8g2_font_haxrcorp4089_tr);
        } else {
            u8g2_SetFont(&u8g2, u8g2_font_profont15_tr);
        }
        int y = 20 + (i * 14);  // Lines at 22, 36, 50 => text just above
        u8g2_DrawStr(&u8g2, 0, y, disp_text);
    }

    // Render the top info bar with WiFi and Time status
    render_top_info_bar(&u8g2, wifi_status, time_status);

    // Send the buffer to the display
    u8g2_SendBuffer(&u8g2);

    return ESP_OK;
}

/**
 * @brief Displays the idle clock screen using current system time.
 *
 * This function retrieves the actual system time and formats it to display as HH:MM.
 * It draws the main time and date on the screen and also renders the top info bar
 * showing WiFi and time status using render_top_info_bar(). The small decorative
 * rectangle frames have been removed.
 *
 * @param u8g2 Pointer to an initialized u8g2 display context.
 * @param wifi_status 0 if WiFi is OK; non-zero otherwise.
 * @param time_status 0 if time is OK; non-zero otherwise.
 */
void display_idle_clock_screen(u8g2_t *u8g2, uint8_t wifi_status, uint8_t time_status)
{
    // Retrieve current system time
    time_t now = time(NULL);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    // Format main time string as "HH:MM" (e.g., "18:30")
    char main_time[16];
    snprintf(main_time, sizeof(main_time), "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

    // Format date string as "Weekday dd.mm" (e.g., "Wednesday 11.12")
    char date_str[32];
    strftime(date_str, sizeof(date_str), "%A %d.%m", &timeinfo);

    // Clear display and set drawing modes
    u8g2_ClearBuffer(u8g2);
    u8g2_SetBitmapMode(u8g2, 1);
    u8g2_SetFontMode(u8g2, 1);

    // Render the top info bar to show WiFi and Time status
    render_top_info_bar(u8g2, wifi_status, time_status);

    // If system time is not set (tm_year < 70 indicates before 1970), show error message
    if(timeinfo.tm_year < 71)
    {
        ESP_LOGW(TAG, "Time not set ");
        u8g2_SetFont(u8g2, u8g2_font_timR14_tr);
        u8g2_DrawStr(u8g2, 10, 30, "Time not set");
        u8g2_DrawStr(u8g2, 10, 50, "Check WiFi");


    }
    else{
    // Draw the main time as HH:MM using a large font
    u8g2_SetFont(u8g2, u8g2_font_timR24_tr);
    u8g2_DrawStr(u8g2, 1, 43, main_time);

    // Draw the date string using a small font
    u8g2_SetFont(u8g2, u8g2_font_timR10_tr);
    u8g2_DrawStr(u8g2, 4, 60, date_str);

    // Draw a horizontal separator line
    u8g2_DrawLine(u8g2, 0, 46, 116, 46);
    }
    // Send the buffer to the display
    u8g2_SendBuffer(u8g2);
}