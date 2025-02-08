#ifndef BUZZER_H
#define BUZZER_H

#include <stdio.h>
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO          (10) // Define the output GPIO
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_DUTY               (4096) // Set duty to 50%. (2 ** 13) * 50% = 4096
#define LEDC_FREQUENCY          (4000) // Frequency in Hertz. Set frequency at 4 kHz

void buzzer_init(void);
void play_tone(uint32_t freq, uint32_t duration_ms);
void play_chirp(uint8_t chirpID);
void play_chirp_task(void *pvParameters);
#define SEND_CHIRP(queue, code)    \
    do {                           \
        uint8_t __chirp = (uint8_t)(code); \
        xQueueSend((queue), &__chirp, 0);    \
    } while(0)


#define NOTE_C3  262
#define NOTE_CS3 277
#define NOTE_D3  294
#define NOTE_DS3 311
#define NOTE_E3  330
#define NOTE_F3  349
#define NOTE_FS3 370
#define NOTE_G3  392
#define NOTE_GS3 415
#define NOTE_A3  440
#define NOTE_AS3 466
#define NOTE_B3  494

#define NOTE_C4  523
#define NOTE_CS4 554
#define NOTE_D4  587
#define NOTE_DS4 622
#define NOTE_E4  659
#define NOTE_F4  698
#define NOTE_FS4 740
#define NOTE_G4  784
#define NOTE_GS4 831
#define NOTE_A4  880
#define NOTE_AS4 932
#define NOTE_B4  988

#define NOTE_C5  1047
#define NOTE_CS5 1109
#define NOTE_D5  1175
#define NOTE_DS5 1245
#define NOTE_E5  1319
#define NOTE_F5  1397
#define NOTE_FS5 1480
#define NOTE_G5  1568
#define NOTE_GS5 1661
#define NOTE_A5  1760
#define NOTE_AS5 1865
#define NOTE_B5  1976

#define NOTE_C6  2093
#define NOTE_CS6 2217
#define NOTE_D6  2349
#define NOTE_DS6 2489
#define NOTE_E6  2637
#define NOTE_F6  2794
#define NOTE_FS6 2960
#define NOTE_G6  3136
#define NOTE_GS6 3322
#define NOTE_A6  3520
#define NOTE_AS6 3729
#define NOTE_B6  3951

#endif // BUZZER_H