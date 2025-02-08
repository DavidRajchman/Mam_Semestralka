#ifndef WIFI_TIME_H
#define WIFI_TIME_H


#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "nvs_flash.h"
#include <string.h>
#include <time.h>

//CONFIGURATION
#define TIME_VALIDITY_MINUTES 1 // Time is considered valid for 15 minutes after synchronization
#define TIMEZONE "CET-1CEST,M3.5.0,M10.5.0/3" //timezone env - set in wifi init

// WiFi status definitions
#define WIFI_STATUS_CONNECTED                  0
#define WIFI_STATUS_DISCONNECTED_SUCCESS       1
#define WIFI_STATUS_DISCONNECTED_FAIL          2


esp_err_t wifi_init();

esp_err_t wifi_connect_sta_simple(const char* ssid, const char* password);

esp_err_t init_sntp(void);

void wifi_sync_sntp_time_once_task(void *param);
int get_time_validity(void);
int get_wifi_status(void);




#endif // WIFI_TIME_H