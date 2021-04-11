#include <sys/cdefs.h>
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#define BUF_SIZE (1024)
#define EVENT_UART_DONE BIT0

static EventGroupHandle_t s_uart_event_group;

static float last_read_temperature = -1.0f;
static float last_read_humidity = -1.0f;

_Noreturn static void echo_task(void *arg)
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

    printf("UART started.");

    while (1) {
        // Read data from the UART
        int len = uart_read_bytes(CONFIG_ESP_UART_PORT_NUM, data, BUF_SIZE, 20 / portTICK_RATE_MS);
        // Write data back to the UART

        // Parse: BEGIN,26034,30000
        if(len > 5) {
            data[len] = 0;
            const char *cdata = (const char *)data;
            printf("Received %s\n", cdata);
            if (strncmp("BEGIN", cdata, 5) == 0) {
                char* substr = malloc(len);
                strncpy(substr, cdata + 6, len);

                char *p = strtok(substr, ",");
                if(p) {
                    long temp = strtol(p, NULL, 10);
                    last_read_temperature = (float)temp * 175.0f / 65535.0f - 45.0f;

                    p = strtok(NULL, ",");
                    if(p) {
                        temp = strtol(p, NULL, 10);
                        last_read_humidity = (float)temp * 100.0f / 65535.0f;

                        printf("Found temp = %f and humidity = %f.", last_read_temperature, last_read_humidity);
                        xEventGroupSetBits(s_uart_event_group, EVENT_UART_DONE);
                        vTaskDelay(portMAX_DELAY); // wait for infinity, because this task is done.
                    }
                }

                free(substr);
            }
        }
    }
}

void uart_start_task(void)
{
    s_uart_event_group = xEventGroupCreate();
    xTaskCreate(echo_task, "uart_echo_task", CONFIG_ESP_UART_TASK_STACK_SIZE, NULL, 10, NULL);
}

void uart_wait_until_done() {
    xEventGroupWaitBits(s_uart_event_group, EVENT_UART_DONE, pdFALSE, pdFALSE, portMAX_DELAY);
}

float uart_get_temperature() {
    return last_read_temperature;
}

float uart_get_humidity() {
    return last_read_humidity;
}
