#include "wifi_time.h"

static esp_netif_t *esp_netif;

// Global variable storing the last successful time sync (UNIX timestamp)
static time_t last_time_sync = 0;

// Global variable for WiFi status, initialized to "fail" until proven otherwise.
static int wifi_status_var = WIFI_STATUS_DISCONNECTED_FAIL;
// Flag to indicate if a connection was ever successful.
static bool last_wifi_successful = false;


static const char *TAG = "WIFI_TIME";

static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        // If the connection was successful before, mark as disconnected (successful)
        if (last_wifi_successful) {
            wifi_status_var = WIFI_STATUS_DISCONNECTED_SUCCESS;
        } else {
            wifi_status_var = WIFI_STATUS_DISCONNECTED_FAIL;
        }
        ESP_LOGI(TAG, "WiFi disconnected. Status: %d", wifi_status_var);
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
        wifi_status_var = WIFI_STATUS_CONNECTED;
        last_wifi_successful = true;
    }
}

esp_err_t wifi_init()
{
    setenv("TZ", TIMEZONE, 1);
    esp_err_t ret = esp_netif_init();
    if (ret != ESP_OK) return ret;
    ret = esp_event_loop_create_default();
    if (ret != ESP_OK) return ret;

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    return ESP_OK;
}


esp_err_t wifi_connect_sta_simple(const char* ssid, const char* password)
{
    ESP_LOGI(TAG, "WiFi connection start");

    esp_netif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) return ret;



    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));

    ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret != ESP_OK) return ret;

    ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (ret != ESP_OK) return ret;

    ret = esp_wifi_start();
    if (ret != ESP_OK) return ret;

    ESP_LOGI(TAG, "wifi_init_sta finished");
    return ESP_OK;
}

static void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Time synchronization completed");
}

esp_err_t init_sntp(void)
{
    if (wifi_status_var != WIFI_STATUS_CONNECTED) {
        ESP_LOGE(TAG, "WiFi not connected - cannot initialize SNTP");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Initializing SNTP");
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    esp_sntp_init();

    // Wait for time to be synchronized
    int retry = 0;
    const int retry_count = 10;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    if (retry == retry_count) {
        ESP_LOGE(TAG, "Failed to sync time via SNTP");

        return ESP_FAIL;
    }

    return ESP_OK;
}

/**
 * @brief Update the time sync timestamp.
 *
 * This function should be called after a successful SNTP time sync.
 */
void update_time_sync_timestamp(void)
{
    last_time_sync = time(NULL);
    ESP_LOGI(TAG, "Time sync timestamp updated");
}


/**
 * @brief Powers on WiFi, connects using the given SSID and password, synchronizes time via SNTP,
 *        then powers off the WiFi modem. Aborts if the total operation exceeds 10 minutes.
 *
 * @param param Pointer to an array of two strings:
 *              - param[0]: The WiFi SSID
 *              - param[1]: The WiFi password
 */
void wifi_sync_sntp_time_once_task(void *param)
{
    const char *ssid = (const char *)((char**)param)[0];
    const char *pass = (const char *)((char**)param)[1];
    TickType_t start_tick = xTaskGetTickCount();
    TickType_t timeout_ticks = pdMS_TO_TICKS(600000); // 10 minutes

    ESP_LOGI(TAG, "WiFi-SNTP task started");

    // Power on and connect WiFi
    esp_err_t ret = wifi_connect_sta_simple(ssid, pass);
    if ( ret != ESP_OK) {
        //print error reason
        ESP_LOGE(TAG, "WiFi init failed: %s", esp_err_to_name(ret));
        goto cleanup;
    }

    // Wait up to 30 seconds for WiFi to connect
    {
        int checks = 30;
        while (wifi_status_var != WIFI_STATUS_CONNECTED && checks-- > 0) {
            // If overall task time is already beyond 10 minutes, abort
            if (xTaskGetTickCount() - start_tick > timeout_ticks) {
                ESP_LOGE(TAG, "WiFi-SNTP task timed out during connection");
                goto cleanup;
            }
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        if (wifi_status_var != WIFI_STATUS_CONNECTED) {
            ESP_LOGE(TAG, "WiFi connection timed out");
            last_wifi_successful = false;
            goto cleanup;
        }
    }

    // Sync time
    if (wifi_status_var == WIFI_STATUS_CONNECTED)
    {
        esp_err_t ret = init_sntp();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "SNTP init failed: %s", esp_err_to_name(ret));
            goto cleanup;
        }
        else {
            ESP_LOGI(TAG, "SNTP sync successful");
            update_time_sync_timestamp();
        }
        // Check total timeout
        if (xTaskGetTickCount() - start_tick > timeout_ticks) {
            ESP_LOGE(TAG, "WiFi-SNTP task timed out after SNTP init");
            goto cleanup;
        }
    }

cleanup:
    //stop the SNTP client
    ESP_LOGI(TAG, "Stopping SNTP client");
    esp_sntp_stop();
    // Disconnect and power off WiFi
    ESP_LOGI(TAG, "Stopping WiFi and powering off modem");
    esp_wifi_stop();
    vTaskDelay(pdMS_TO_TICKS(2000));
    esp_netif_destroy(esp_netif);
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_deinit());
    vTaskDelay(pdMS_TO_TICKS(1000));

    ESP_LOGI(TAG, "WiFi-SNTP task finished");
    vTaskDelete(NULL);
}


/**
 * @brief Check if the system time is valid.
 *
 * Returns 0 if the last time sync was less than TIME_VALIDITY_MINUTES old,
 * otherwise returns 1 indicating the time sync is outdated.
 *
 * @return int 0 if valid, 1 if outdated.
 */
int get_time_validity(void)
{
    time_t now = time(NULL);
    double diff_secs = difftime(now, last_time_sync);
    //if year is less than 2000, time is not set
    if (localtime(&now)->tm_year < 90) {
        return 1;  // Time sync is outdated
    }
    if (diff_secs < (TIME_VALIDITY_MINUTES * 60)) {
        return 0;  // Time sync is valid
    }
    return 1;      // Time sync is outdated
}

/* get_wifi_status():
   Returns:
     WIFI_STATUS_CONNECTED (0) if WiFi is connected,
     WIFI_STATUS_DISCONNECTED_SUCCESS (1) if WiFi is disconnected/disabled but last connection was successful,
     WIFI_STATUS_DISCONNECTED_FAIL (2) if WiFi is disconnected/disabled and last connection attempt was not successful.
*/
int get_wifi_status(void)
{
    return wifi_status_var;
}