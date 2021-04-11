
#include <string.h>
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_wifi.h"

#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 2048
static const char *TAG = "http";

esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
    return ESP_OK;
}

void http_rest_with_url(float temperature, float humidity, int8_t rssi, uint32_t powerVoltage, uint8_t uartOk) {
    ESP_LOGI(TAG, "1");

    char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
    char* query = malloc(512);

    //Get MAC address for WiFi Station interface
    uint8_t mac[6] = {0};
    ESP_ERROR_CHECK(esp_read_mac(mac, ESP_MAC_WIFI_STA));

    // - QUERY -
    ESP_LOGI(TAG, "2");
    sprintf(query, "t=%f&h=%f&mac=%02x:%02x:%02x&rssi=%d&v=%i&uart=%i",
            temperature, humidity,
            mac[3], mac[4], mac[5],
            rssi,
            powerVoltage,
            uartOk
    );

    ESP_LOGI(TAG, "3");
    esp_http_client_config_t config = {
        .host = "192.168.1.121",
        .port = 8888,
        .path = "/",
        .query = query,
        //.event_handler = _http_event_handler,
        //.user_data = local_response_buffer,        // Pass address of local buffer to get response
        .disable_auto_redirect = true,
        .keep_alive_enable = false,
        .timeout_ms = 5000
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    // GET

    ESP_LOGI(TAG, "4");
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }

    ESP_LOGI(TAG, "5");
    //ESP_LOG_BUFFER_HEX(TAG, local_response_buffer, strlen(local_response_buffer));
}
