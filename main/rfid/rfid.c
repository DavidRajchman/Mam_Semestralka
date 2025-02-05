#include "rfid.h"
static const char *TAG = "rfid";



rc522_driver_handle_t driver;
rc522_handle_t scanner;
rc522_spi_config_t driver_config = {
    .host_id = SPI2_HOST,
    .bus_config = &(spi_bus_config_t){
        .miso_io_num = RC522_SPI_BUS_GPIO_MISO,
        .mosi_io_num = RC522_SPI_BUS_GPIO_MOSI,
        .sclk_io_num = RC522_SPI_BUS_GPIO_SCLK,
    },
    .dev_config = {
        .spics_io_num = RC522_SPI_SCANNER_GPIO_SDA,
    },
    .rst_io_num = RC522_SCANNER_GPIO_RST,
};


/**
 * @brief Initialize and configure RFID reader for PICC events.
 *
 * This function sets up the RFID scanner by configuring the SPI interface and driver,
 * registers a handler for PICC state change events, and starts the scanner.
 *
 * SPI parameters can be adjusted in header file
 *
 * @param on_picc_state_changed_handler Pointer to the event handler function defined in main.
 */
void rfid_setup(void (*on_picc_state_changed_handler)(void *arg, esp_event_base_t base, int32_t event_id, void *data))
{
    rc522_spi_create(&driver_config, &driver);
    rc522_driver_install(driver);

    rc522_config_t scanner_config = {
        .driver = driver,
    };

    rc522_create(&scanner_config, &scanner);
    rc522_register_events(scanner, RC522_EVENT_PICC_STATE_CHANGED, on_picc_state_changed_handler, NULL);
    rc522_start(scanner);
}



/**
 * @brief Convert a UID to a hexadecimal string without any spaces.
 *
 * The resulting string will contain two hexadecimal characters per byte,
 * and must be allocated with at least (uid->length * 2 + 1) bytes.
 *
 * @param uid Pointer to the UID structure.
 * @param out_str Output buffer to store the resulting string.
 * @param out_size The size of the output buffer.
 * @return true on success, false if the buffer is too small.
 */
bool uid_to_str_no_space(const rc522_picc_uid_t *uid, char *out_str, size_t out_size)
{
    if (!uid || !out_str)
        return false;

    // Calculate the required size (2 hex digits per byte plus null terminator)
    size_t required_size = uid->length * 2 + 1;
    if (out_size < required_size)
        return false;

    size_t offset = 0;
    for (uint32_t i = 0; i < uid->length; i++) {
        // Write two hex characters per byte without spaces.
        offset += snprintf(out_str + offset, out_size - offset, "%02X", uid->value[i]);
    }
    out_str[offset] = '\0';
    return true;
}