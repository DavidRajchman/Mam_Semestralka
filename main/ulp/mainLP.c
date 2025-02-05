#include <stdint.h>
#include <stdbool.h>
#include "ulp_lp_core.h"
#include "ulp_lp_core_utils.h"
#include "ulp_lp_core_gpio.h"

#define DEBOUNCE_THRESHOLD    10    // Minimum cycles to consider a valid press
#define LONG_PRESS_THRESHOLD  100   // Number of cycles to consider a long press - depends on the LP timer period (5 ms)

#define INTERUPT_PIN_LP 0

#define BUTTON1_PIN_LP 4
#define BUTTON2_PIN_LP 5
#define BUTTON3_PIN_LP 6
#define BUTTON4_PIN_LP 7

uint16_t press_counter[4] = {0};
uint32_t gpio_values[4] = {0};
uint8_t gpio_values_output[4] = {0};
uint8_t send_interupt_flag = 0;

// This 32-bit variable combines the four 8-bit outputs.
volatile uint32_t gpio_values_single_var = 0;
volatile uint8_t button_pressed_num = 0;

void send_interupt_to_HP(void)
{
    // Toggle interrupt pin:
    ulp_lp_core_gpio_set_level(INTERUPT_PIN_LP, 1);
    ulp_lp_core_gpio_set_level(INTERUPT_PIN_LP, 0);
}

int main (void)
{
    // Read current button states for 4 buttons.
    gpio_values[0] = ulp_lp_core_gpio_get_level(BUTTON1_PIN_LP);
    gpio_values[1] = ulp_lp_core_gpio_get_level(BUTTON2_PIN_LP);
    gpio_values[2] = ulp_lp_core_gpio_get_level(BUTTON3_PIN_LP);
    gpio_values[3] = ulp_lp_core_gpio_get_level(BUTTON4_PIN_LP);

    for (int i = 0; i < 4; i++)
    {
        // If button is pressed (active low)
        if (gpio_values[i] == 0)
        {
            // Increase press duration counter.
            if (press_counter[i] < 0xFFFF)
            {
                press_counter[i]++;
            }
        }
        else  // Button is released (logic high)
        {
            // Only trigger an event if the button was pressed long enough.
            if (press_counter[i] >= DEBOUNCE_THRESHOLD)
            {
                // Assign a value: 2 for a long press, else 1 for short press.
                gpio_values_output[i] = (press_counter[i] >= LONG_PRESS_THRESHOLD) ? 2 : 1;
                button_pressed_num = i + 1;
                send_interupt_flag++;
            }
            // Reset press counter for this button.
            press_counter[i] = 0;
        }
    }

    // Send interrupt to HP_core when at least one valid (debounced) button press is detected.
    if (send_interupt_flag == 1)
    {
        send_interupt_to_HP();
        gpio_values_single_var = 0;
        // Single button: assign its value in the highest-order byte.
        gpio_values_single_var = (gpio_values_output[0] << 24) |
                                  (gpio_values_output[1] << 16) |
                                  (gpio_values_output[2] << 8)  |
                                  (gpio_values_output[3]);
    }
    else if (send_interupt_flag > 1)
    {
        // Combine the 4 8-bit outputs into a 32-bit variable.
        gpio_values_single_var = (gpio_values_output[0] << 24) |
                                  (gpio_values_output[1] << 16) |
                                  (gpio_values_output[2] << 8)  |
                                  (gpio_values_output[3]);
        // For multiple buttons pressed, use a special value (e.g., button_pressed_num = 10).
        button_pressed_num = 10;
        send_interupt_to_HP();
    }
    send_interupt_flag = 0;

    return 0;
}