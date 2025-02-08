#include "buzzer.h"
static void buzzer_init(void)
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .duty_resolution  = LEDC_DUTY_RES,
        .timer_num        = LEDC_TIMER,
        .freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 4 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LEDC_OUTPUT_IO,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}



void play_tone(uint32_t freq, uint32_t duration_ms)
{
    ledc_set_freq(LEDC_MODE, LEDC_TIMER, freq);
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, LEDC_DUTY);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
    vTaskDelay(duration_ms / portTICK_PERIOD_MS);
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}


void play_chirp(uint8_t chirpID)
{
    switch (chirpID)
    {
    case 1:
        play_tone(NOTE_C4, 200);
        vTaskDelay(50 / portTICK_PERIOD_MS); // Quiet time
        play_tone(NOTE_E4, 200);
        vTaskDelay(50 / portTICK_PERIOD_MS); // Quiet time
        play_tone(NOTE_G4, 200);
        break;
    case 2:
        play_tone(NOTE_G4, 300);
        vTaskDelay(100 / portTICK_PERIOD_MS); // Quiet time
        play_tone(NOTE_C4, 300);
        break;
    case 3:
        play_tone(NOTE_C4, 150);
        vTaskDelay(25 / portTICK_PERIOD_MS); // Quiet time
        play_tone(NOTE_E4, 150);
        vTaskDelay(25 / portTICK_PERIOD_MS); // Quiet time
        play_tone(NOTE_G4, 150);
        vTaskDelay(25 / portTICK_PERIOD_MS); // Quiet time
        play_tone(NOTE_B4, 150);
        break;
    case 4:
        play_tone(NOTE_A4, 250);
        vTaskDelay(50 / portTICK_PERIOD_MS); // Quiet time
        play_tone(NOTE_C4, 250);
        vTaskDelay(50 / portTICK_PERIOD_MS); // Quiet time
        play_tone(NOTE_E4, 250);
        break;
    case 5:
        play_tone(NOTE_C4, 200);
         vTaskDelay(50 / portTICK_PERIOD_MS); // Quiet time
        play_tone(NOTE_G4, 200);
         vTaskDelay(50 / portTICK_PERIOD_MS); // Quiet time
        play_tone(NOTE_C5, 200);
         vTaskDelay(50 / portTICK_PERIOD_MS); // Quiet time
        play_tone(NOTE_G4, 200);
         vTaskDelay(50 / portTICK_PERIOD_MS); // Quiet time
        play_tone(NOTE_C4, 200);
        break;
    default:
        break;
    }
}

