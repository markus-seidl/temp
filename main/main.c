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

void app_main(void)
{
#if CONFIG_PM_ENABLE
    // Configure dynamic frequency scaling:
    // maximum and minimum frequencies are set in sdkconfig,
    // automatic light sleep is enabled if tickless idle support is enabled.
#if CONFIG_IDF_TARGET_ESP32
    esp_pm_config_esp32_t pm_config = {
#elif CONFIG_IDF_TARGET_ESP32S2
            esp_pm_config_esp32s2_t pm_config = {
#elif CONFIG_IDF_TARGET_ESP32C3
    esp_pm_config_esp32c3_t pm_config = {
#endif
            .max_freq_mhz = 80,
            .min_freq_mhz = 80,
#if CONFIG_FREERTOS_USE_TICKLESS_IDLE
            // .light_sleep_enable = true
#endif
    };
    printf("Enable dynamic light sleep...");
    ESP_ERROR_CHECK(esp_pm_configure(&pm_config));
#endif // CONFIG_PM_ENABLE

    // ------------- Enable wakeup

    /* Configure the button GPIO as input, enable wakeup */
    const int button_gpio_num = 2;
//    const int wakeup_level = GPIO_INTR_HIGH_LEVEL;
//    const gpio_config_t config = {
//            .pin_bit_mask = BIT(button_gpio_num),
//            .mode = GPIO_MODE_INPUT,
//    };
//    ESP_ERROR_CHECK(gpio_config(&config));
//    ESP_ERROR_CHECK(esp_deep_sleep_enable_gpio_wakeup(BIT(button_gpio_num), ESP_GPIO_WAKEUP_GPIO_HIGH));
//    printf("Enabling GPIO wakeup on pins GPIO%d\n", button_gpio_num);
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
        printf("POWER DOWN\n");
        uart_wait_tx_idle_polling(0);
        fflush(stdout);
        esp_deep_sleep_start();
    }

    //----------------------------

    printf("Hello world!\n");

    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), WiFi%s%s, ",
            CONFIG_IDF_TARGET,
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    printf("silicon revision %d, ", chip_info.revision);

    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Minimum free heap size: %d bytes\n", esp_get_minimum_free_heap_size());

    uart_start_task();


    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init_sta();

    uart_wait_until_done();

    ESP_LOGI("main", "Before HTTP request");
    http_rest_with_url(uart_get_temperature(), uart_get_humidity());

    ESP_LOGI("main", "Stop WIFI");
    wifi_stop();

    fflush(stdout);

    esp_deep_sleep_start();
}
