#include "buzzer.h"
void buzzer_init(void)
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

/**
 * @brief BLOCKING Play a chirp sound based on the chirpID
 * TASK BLOCKS EXECUTION FOR DURATION OF THE CHRIP
 * use play_chirp_task in combination with SEND_CHIRP(chirpQueue, X) macro
 * @param chirpID The ID of the chirp to play
 **/
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
    case 2: //failure chirp
        play_tone(NOTE_E3, 150);
        vTaskDelay(300 / portTICK_PERIOD_MS); // Quiet time
        play_tone(NOTE_C3, 100);
        vTaskDelay(30 / portTICK_PERIOD_MS); // Quiet time
        play_tone(NOTE_A3, 150);
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
    case 4: //best chirp
        play_tone(NOTE_A3, 150);
        vTaskDelay(300 / portTICK_PERIOD_MS); // Quiet time
        play_tone(NOTE_C3, 100);
        vTaskDelay(30 / portTICK_PERIOD_MS); // Quiet time
        play_tone(NOTE_E3, 150);
        break;
    case 5: //rfid tag read chirp
        play_tone(NOTE_C5, 50);
        break;
    default:
        break;
    }
}

void play_chirp_task(void *pvParameters)
{
    QueueHandle_t chirpQueue = (QueueHandle_t)pvParameters;
    uint8_t chirpCode;

    ESP_LOGI("BUZZER_TASK", "Play Chirp Task started");
    while (1) {
        if (xQueueReceive(chirpQueue, &chirpCode, portMAX_DELAY) == pdPASS) {
            ESP_LOGI("BUZZER_TASK", "Received chirp code: %d", chirpCode);
            play_chirp(chirpCode);
        }
    }
}