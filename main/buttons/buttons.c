#include "buttons.h"

// Duplicate the definition from main if not already available.


#include "buttons.h"


// Global storage for quick action tasks for each button.
static quick_action_task_func_t quick_action_task_functions[4] = { NULL, NULL, NULL, NULL };

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
            if (pinNumber == BUTTONS_INTERUPT_PIN)
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
                            else
                            {
                                quick_action_task_launcher(i);
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
 * @brief Initialize the button/ULP interrupt processing and assign quick action tasks.
 *
 * This function installs the GPIO ISR, creates an internal interrupt queue, and
 * stores the supplied quick action task functions (each of which must follow the standard
 * FreeRTOS task prototype). When button control is not active, the corresponding quick
 * action task will be launched upon button press.
 *
 * @param buttonControlQueue The queue for button control commands.
 * @param task0 Quick action task function for button 0.
 * @param task1 Quick action task function for button 1.
 * @param task2 Quick action task function for button 2.
 * @param task3 Quick action task function for button 3.
 */
void init_buttons(QueueHandle_t buttonControlQueue, quick_action_task_func_t task0,
                  quick_action_task_func_t task1, quick_action_task_func_t task2, quick_action_task_func_t task3)
{
    install_buttons_isr(BUTTONS_INTERUPT_PIN);
    buttonControlQueue_local = buttonControlQueue;
    // Store quick action task functions.
    quick_action_task_functions[0] = task0;
    quick_action_task_functions[1] = task1;
    quick_action_task_functions[2] = task2;
    quick_action_task_functions[3] = task3;

    interruptQueue = xQueueCreate(10, sizeof(int));
    if (interruptQueue == NULL)
    {
        ESP_LOGE(TAG, "Failed to create interruptQueue");
    }

    xTaskCreate(buttons_interrupt_task, "buttons_interrupt_task", 2048, NULL, 1, NULL);
}

/**
 * @brief Launches a quick action task corresponding to the given button.
 *
 * This function creates and starts a new FreeRTOS task based on the quick action task
 * function pointer supplied via init_buttons. The new task is launched with a standard
 * FreeRTOS function prototype.
 *
 * @param button_id The button index (0 through 3).
 */
void quick_action_task_launcher(int button_id) {
    if (button_id < 0 || button_id > 3) {
        ESP_LOGE(TAG, "Invalid button_id %d for quick action task launcher", button_id);
        return;
    }
    if (quick_action_task_functions[button_id] != NULL) {
        BaseType_t ret = xTaskCreate(
            quick_action_task_functions[button_id],
            "quick_action_task",   // Task name
            4096,                  // Stack size
            NULL,                  // Parameter for the task
            1,                     // Priority (adjust as needed)
            NULL
        );
        if (ret != pdPASS) {
            ESP_LOGE(TAG, "Failed to create quick action task for button %d", button_id);
        }
    } else {
        ESP_LOGW(TAG, "No quick action task assigned for button %d", button_id);
    }
}