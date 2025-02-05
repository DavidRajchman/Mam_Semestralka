#ifndef RFID_H
#define RFID_H

#include "rc522.h"
#include "driver/rc522_spi.h"
#include "rc522_picc.h"
#include "esp_log.h"

#define RC522_SPI_BUS_GPIO_MISO    (18)
#define RC522_SPI_BUS_GPIO_MOSI    (19)
#define RC522_SPI_BUS_GPIO_SCLK    (20)
#define RC522_SPI_SCANNER_GPIO_SDA (21)
#define RC522_SCANNER_GPIO_RST     (-1) // soft-reset

void rfid_setup(void (*on_picc_state_changed_handler)(void *arg, esp_event_base_t base, int32_t event_id, void *data));
bool uid_to_str_no_space(const rc522_picc_uid_t *uid, char *out_str, size_t out_size);


#endif