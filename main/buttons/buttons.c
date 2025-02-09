#include "buttons.h"

// Duplicate the definition from main if not already available.
#ifndef INTERUPT_PIN
#define INTERUPT_PIN 1
#endif

// External variables from the ULP program and main logic.
extern uint32_t ulp_button_pressed_num;
extern uint32_t ulp_gpio_values_single_var;
extern uint8_t button_control_active; // remains defined in main

static const char *TAG = "BUTTONS";

// Internal queue for GPIO interrupt events.
static QueueHandle_t interruptQueue = NULL;
// Local copy of the buttonControlQueue provided from main.
static QueueHandle_t buttonControlQueue_local = NULL;
// Local storage for button values as used in main.
static uint8_t button_values[4] = {0};

static void IRAM_ATTR buttons_gpio_isr_handler(void *args)
{
    int pinNumber = (int)args;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(interruptQueue, &pinNumber, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken)
    {
        portYIELD_FROM_ISR();
    }
}

// Task that processes ULP events from the interrupt queue.
static void buttons_interrupt_task(void *params)
{
    int pinNumber;
    while (true)
    {
        if (xQueueReceive(interruptQueue, &pinNumber, portMAX_DELAY))
        {
            // Process ULP-related events when the designated interrupt pin fires.
            if (pinNumber == INTERUPT_PIN)
            {
                if (ulp_button_pressed_num == 0)
                {
                    ESP_LOGW(TAG, "No button pressed, but interrupt received");
                }
                if (ulp_button_pressed_num > 0)
                {
                    //ESP_LOGI(TAG, "Multiple buttons pressed");
                    button_values[0] = (ulp_gpio_values_single_var >> 24) & 0xFF;
                    button_values[1] = (ulp_gpio_values_single_var >> 16) & 0xFF;
                    button_values[2] = (ulp_gpio_values_single_var >> 8) & 0xFF;
                    button_values[3] = ulp_gpio_values_single_var & 0xFF;
                    ulp_gpio_values_single_var = 0;
                    for (int i = 0; i < 4; i++)
                    {
                        if (button_values[i] != 0)
                        {
                            if (button_values[i] == 1)
                            {
                                ESP_LOGI(TAG, "Button %d pressed", i);
                            }
                            else if (button_values[i] == 2)
                            {
                                ESP_LOGI(TAG, "Button %d long pressed", i);
                            }
                            if (button_control_active == 1)
                            {
                                button_control_t button_control;
                                button_control.button_id = i;
                                button_control.command = button_values[i];
                                xQueueSend(buttonControlQueue_local, &button_control, 0);
                            }
                        }
                    }

                }
            }
        }
    }
}


/**
 * @brief Install a GPIO ISR for button interrupts.
 *
 * @param button_gpio The GPIO number to install the interrupt for.
 */
static void install_buttons_isr(uint32_t button_gpio)
{
    esp_err_t err = gpio_install_isr_service(0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install GPIO ISR service: %s", esp_err_to_name(err));
        // Consider returning or handling the error appropriately
    }
    gpio_set_direction(button_gpio, GPIO_MODE_INPUT);
    gpio_pulldown_dis(button_gpio);
    gpio_pullup_dis(button_gpio);
    gpio_set_intr_type(button_gpio, GPIO_INTR_POSEDGE);
    ESP_ERROR_CHECK(gpio_isr_handler_add(button_gpio, buttons_gpio_isr_handler, (void *)button_gpio));
}

/**
 * @brief Initialize the button/ULP interrupt processing.
 *
 * Creates an internal interrupt queue and starts the processing task.
 * The buttonControlQueue (declared in main) is passed in so that processed
 * button events can be forwarded.
 *
 * @param buttonControlQueue The queue for button control commands.
 */
void init_buttons(QueueHandle_t buttonControlQueue)
{
    install_buttons_isr(BUTTONS_INTERUPT_PIN);
    buttonControlQueue_local = buttonControlQueue;
    interruptQueue = xQueueCreate(10, sizeof(int));
    if (interruptQueue == NULL)
    {
        ESP_LOGE(TAG, "Failed to create interruptQueue");
    }

    xTaskCreate(buttons_interrupt_task, "buttons_interrupt_task", 2048, NULL, 1, NULL);
}