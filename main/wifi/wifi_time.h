#ifndef WIFI_TIME_H
#define WIFI_TIME_H


#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "nvs_flash.h"
#include <string.h>

esp_err_t wifi_init_sta_simple(const char* ssid, const char* password);

esp_err_t init_sntp(void);


#endif // WIFI_TIME_H