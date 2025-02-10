#include "led.h"
static led_strip_handle_t led_strip;

void init_led()
{
    /// LED strip common configuration
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_GPIO,  // The GPIO that connected to the LED strip's data line
        .max_leds = 1,                 // The number of LEDs in the strip,
        .led_model = LED_MODEL_WS2812, // LED strip model, it determines the bit timing
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB, // The color component format is G-R-B
        .flags = {
            .invert_out = false, // don't invert the output signal
        }
    };

    /// RMT backend specific configuration
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,    // different clock source can lead to different power consumption
        .resolution_hz = 10 * 1000 * 1000, // RMT counter clock frequency: 10MHz
        .mem_block_symbols = 64,           // the memory size of each RMT channel, in words (4 bytes)
        .flags = {
            .with_dma = false, // DMA feature is available on chips like ESP32-S3/P4
        }
    };

    /// Create the LED strip object

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    ESP_ERROR_CHECK(led_strip_clear(led_strip));

}

void set_led(uint32_t red, uint32_t green, uint32_t blue)
{
    if (!led_strip)
    {
        ESP_LOGE("LED", "LED strip not initialized");
        return;
    }
    ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, 0, red, green, blue));
    ESP_ERROR_CHECK(led_strip_refresh(led_strip));
}