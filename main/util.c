#include "esp_pm.h"
#include "driver/gpio.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "util.h"
#include "esp_timer.h"


#define WAKEUP_GPIO_PIN 2

static const char *TAG = "util";

void enable_sleep() {
    // Configure dynamic frequency scaling:
    // maximum and minimum frequencies are set in sdkconfig,
    // automatic light sleep is enabled if tickless idle support is enabled.
    esp_pm_config_esp32s2_t pm_config = {
            .max_freq_mhz = CONFIG_ESP32S2_DEFAULT_CPU_FREQ_MHZ,
            .min_freq_mhz = CONFIG_ESP32S2_DEFAULT_CPU_FREQ_MHZ,
            .light_sleep_enable = true
    };
    ESP_ERROR_CHECK(esp_pm_configure(&pm_config));
}

uint8_t enable_wakeup_source() {

    /* Configure the button GPIO as input, enable wakeup */
    ESP_LOGI(TAG, "Enabling EXT1 wakeup on pins GPIO%d", WAKEUP_GPIO_PIN);
    esp_sleep_enable_ext1_wakeup(1ULL << WAKEUP_GPIO_PIN, ESP_EXT1_WAKEUP_ANY_HIGH);
    //esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    //gpio_pulldown_en(button_gpio_num);

    if(esp_sleep_is_valid_wakeup_gpio(WAKEUP_GPIO_PIN)) {
        ESP_LOGI(TAG, "GPIO is valid as wakeup GPIO");
    }

    uint64_t wakeup_pin_mask = esp_sleep_get_ext1_wakeup_status();
    if (wakeup_pin_mask != 0) {
        int pin = __builtin_ffsll(wakeup_pin_mask) - 1;
        ESP_LOGI(TAG, "Wake up from GPIO %d\n", pin);

        return WAKEUP_FROM_GPIO;
    } else {
        ESP_LOGI(TAG, "Other wake up source");

        // fflush(stdout);
        // uart_wait_tx_idle_polling(0);

        return WAKEUP_FROM_OTHER;
    }

    //----------------------------
}

int64_t millis() {
    return (esp_timer_get_time() / 1000ULL);
}
