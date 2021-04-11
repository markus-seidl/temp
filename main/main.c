#include <stdio.h>
#include "wifi.h"
#include "http.h"
#include "uart.h"
#include "adc.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_log.h"
#include "esp_pm.h"
#include "driver/gpio.h"
#include "esp_sleep.h"
#include "driver/uart.h"
#include "nvs_flash.h"
#include "util.h"

static const char *TAG = "main";

static RTC_DATA_ATTR float LAST_TEMPERATURE = -1;
static RTC_DATA_ATTR float LAST_HUMIDITY = -1;

static RTC_DATA_ATTR uint8_t STAT_WIFI_ERR = 0;
static RTC_DATA_ATTR uint8_t STAT_HTTP_ERR = 0;
static RTC_DATA_ATTR uint8_t STAT_UART_ERR = 0;
static RTC_DATA_ATTR uint8_t STAT_WAKEUP_CNT = 0;
static RTC_DATA_ATTR int64_t STAT_SEND_MS = 0;

void app_main(void)
{
    STAT_WAKEUP_CNT++;

    ESP_LOGW(TAG, "Enable dynamic light sleep...");
    enable_sleep();

    ESP_LOGI(TAG, "Enable wakeup source / determine wakeup source...");
    uint8_t wakeup_source = enable_wakeup_source();

    ESP_LOGI(TAG, "Start listening on UART...");
    uart_start_task();

    ESP_LOGI(TAG, "Reading power voltage...");
    uint32_t powerVoltage = adc_read_voltage();

    ESP_LOGI(TAG, "Initializing Wifi");
    uint8_t wifiErr = wifi_init_sta();
    if(wifiErr != ESP_OK) {
        STAT_WIFI_ERR++;
    }

    ESP_LOGI(TAG, "Waiting for UART data...");
    uint8_t uartErr = uart_wait_until_done();
    if(uartErr != ESP_OK) {
        STAT_UART_ERR++;
    }

    float temperature = uart_get_temperature();
    float humidity = uart_get_humidity();
    if(uartErr != ESP_OK) {
        temperature = LAST_TEMPERATURE;
        humidity = LAST_HUMIDITY;
    } else {
        LAST_TEMPERATURE = temperature;
        LAST_HUMIDITY = humidity;
    }

    ESP_LOGI(TAG, "Sending data to server...");

    int64_t time_before_entering_loop = millis();
    STAT_SEND_MS += time_before_entering_loop;
    for(int http_retry = 0; http_retry <= 2; http_retry++) {
        STAT_SEND_MS += millis() - time_before_entering_loop;
        esp_err_t httpErr = http_rest_with_url(temperature, humidity, wifi_rssi(), powerVoltage, uartErr,
                                                STAT_WIFI_ERR, STAT_HTTP_ERR, STAT_UART_ERR, STAT_WAKEUP_CNT, STAT_SEND_MS);
        if(httpErr == ESP_OK) {
            LAST_TEMPERATURE = -1.0f;
            LAST_HUMIDITY = -1.0f;

            STAT_WIFI_ERR = 0;
            STAT_HTTP_ERR = 0;
            STAT_UART_ERR = 0;
            STAT_WAKEUP_CNT = 0;
            STAT_SEND_MS = 0;

            break;
        } else {
            STAT_HTTP_ERR++;
            ESP_LOGE(TAG, "Http error, retry.");
        }
    }

    ESP_LOGI(TAG, "Stopping WIFI");
    wifi_stop();

    ESP_LOGI(TAG, "Sleeping... ZzzzZzzzZzzzZzzz");
    esp_deep_sleep_start();
}
