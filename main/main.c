/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS CONFIG_PM_ENABLEOF ANY KIND, either express or implied.
*/
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

static const char *TAG = "main";

static RTC_DATA_ATTR float LAST_TEMPERATURE;
static RTC_DATA_ATTR float LAST_HUMIDITY;

void app_main(void)
{
    // Configure dynamic frequency scaling:
    // maximum and minimum frequencies are set in sdkconfig,
    // automatic light sleep is enabled if tickless idle support is enabled.
    /*esp_pm_config_esp32s2_t pm_config = {
            .max_freq_mhz = CONFIG_ESP32S2_DEFAULT_CPU_FREQ_MHZ,
            .min_freq_mhz = CONFIG_ESP32S2_DEFAULT_CPU_FREQ_MHZ,
            .light_sleep_enable = true
    };
    printf("Enable dynamic light sleep...");
    ESP_ERROR_CHECK(esp_pm_configure(&pm_config));*/

    // ------------- Enable wakeup

    /* Configure the button GPIO as input, enable wakeup */
    const int button_gpio_num = 2;
    printf("Enabling EXT1 wakeup on pins GPIO%d\n", button_gpio_num);
    esp_sleep_enable_ext1_wakeup(1ULL << button_gpio_num, ESP_EXT1_WAKEUP_ANY_HIGH);
    //esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    //gpio_pulldown_en(button_gpio_num);

//    if (gpio_get_level(button_gpio_num) == 1) {
//        printf("WAKEUP HIGH\n");
//    } else {
//        printf("WAKEUP LOW - sleeping!");
//
//        uart_wait_tx_idle_polling(0);
//        fflush(stdout);
//
//        esp_deep_sleep_start();
//    }

    if(esp_sleep_is_valid_wakeup_gpio(button_gpio_num)) {
        printf("GPIO is valid as wakeup gpio.\n");
    }

    uint64_t wakeup_pin_mask = esp_sleep_get_ext1_wakeup_status();
    if (wakeup_pin_mask != 0) {
        int pin = __builtin_ffsll(wakeup_pin_mask) - 1;
        printf("Wake up from GPIO %d\n", pin);
    } else {
        printf("POWER DOWN, but disabled\n");
        uart_wait_tx_idle_polling(0);
        fflush(stdout);
        //esp_deep_sleep_start();
    }

    //----------------------------

    uart_start_task();

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    uint32_t powerVoltage = adc_read_voltage();

    wifi_init_sta();

    uint8_t uartOk = uart_wait_until_done();

    ESP_LOGI(TAG, "Before HTTP request");
    http_rest_with_url(uart_get_temperature(), uart_get_humidity(), wifi_rssi(), powerVoltage, uartOk);

    ESP_LOGI(TAG, "Stop WIFI");
    wifi_stop();

    fflush(stdout);

    esp_deep_sleep_start();
}
