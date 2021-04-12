#include <sys/cdefs.h>
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"

#define BUF_SIZE (1024)
#define EVENT_UART_DONE BIT0

static RTC_DATA_ATTR float LAST_TEMPERATURE = -1;
static RTC_DATA_ATTR float LAST_HUMIDITY = -1;

static const char *TAG = "ht-uart";

static EventGroupHandle_t s_uart_event_group;

static float last_read_temperature = -1.0f;
static float last_read_humidity = -1.0f;

_Noreturn static void uart_recv_task(void *arg)
{
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = CONFIG_ESP_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    int intr_alloc_flags = 0;

#if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif

    ESP_ERROR_CHECK(uart_driver_install(CONFIG_ESP_UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(CONFIG_ESP_UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(CONFIG_ESP_UART_PORT_NUM, CONFIG_ESP_UART_TXD, CONFIG_ESP_UART_RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    // Configure a temporary buffer for the incoming data
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);

    ESP_LOGI(TAG, "UART started");

    while (1) {
        // Read data from the UART
        int len = uart_read_bytes(CONFIG_ESP_UART_PORT_NUM, data, BUF_SIZE, 20 / portTICK_RATE_MS);
        // Write data back to the UART

        // Parse: BEGIN,26034,30000
        if(len > 5) {
            data[len] = 0;
            char *cdata = (char *)data;
            ESP_LOGI(TAG, "Received %s", cdata);

            if (strncmp("BEGIN", cdata, 5) == 0) {
                // There could be multiple BEGINs, try to find the last BEGIN element.
                char *last = strtok(cdata, "BEGIN");
                ESP_LOGI(TAG, "- Part: %s", last);
                while(1) {
                    char* temp = strtok(NULL, "BEGIN");
                    if(temp) {
                        ESP_LOGI(TAG, "- Part: %s\n", temp);
                        last = temp;
                    } else {
                        break;
                    }
                }
                cdata = last;
                ESP_LOGI(TAG,"- Final part (used for processing): %s", cdata);

                char* substr = malloc(len);
                strncpy(substr, cdata + 1, len);

                char *p = strtok(substr, ",");
                if(p) {
                    long temp = strtol(p, NULL, 10);
                    if(temp > 0) {
                        last_read_temperature = (float)temp * 175.0f / 65535.0f - 45.0f;
                    } else {
                        last_read_temperature = -1.0f;
                    }

                    p = strtok(NULL, ",");
                    if(p) {
                        temp = strtol(p, NULL, 10);
                        if(temp > 0) {
                            last_read_humidity = (float)temp * 100.0f / 65535.0f;
                        } else {
                            last_read_humidity = -1.0f;
                        }

                        if(last_read_temperature > 0) {
                            LAST_TEMPERATURE = last_read_temperature;
                        }

                        if(last_read_humidity > 0) {
                            LAST_HUMIDITY = last_read_humidity;
                        }

                        ESP_LOGI(TAG, "Found temp = %f and humidity = %f", last_read_temperature, last_read_humidity);
                        xEventGroupSetBits(s_uart_event_group, EVENT_UART_DONE);
                        vTaskDelay(portMAX_DELAY); // wait for infinity, because this task is done.
                    }
                }
                free(substr);
            }
        }
    }
}

void uart_start_task(void) {
    s_uart_event_group = xEventGroupCreate();
    xTaskCreate(uart_recv_task, "uart_recv_task", CONFIG_ESP_UART_TASK_STACK_SIZE, NULL, 10, NULL);
}

uint8_t uart_wait_until_done() {
    // wait max 1s for uart data to arrive
    EventBits_t eventBits = xEventGroupWaitBits(s_uart_event_group, EVENT_UART_DONE, pdFALSE, pdFALSE, 1000 / portTICK_RATE_MS);

    if((eventBits & EVENT_UART_DONE) != 0) {
        // data received
        ESP_LOGI(TAG, "UART data received");
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "NO UART data received.");

//        // no data found, reset temp / humid
//        last_read_temperature = -1.0f;
//        last_read_humidity = -1.0f;

        return ESP_FAIL;
    }
}

float uart_get_temperature() {
    return LAST_TEMPERATURE;
}

float uart_get_humidity() {
    return LAST_HUMIDITY;
}

void uart_reset_value_buffer() {
    LAST_TEMPERATURE = -1;
    LAST_HUMIDITY = -1;
}
