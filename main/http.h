
#include "esp_err.h"

esp_err_t http_rest_with_url(float temperature, float humidity, int8_t rssi,
                             uint32_t powerVoltage, int8_t uartOk,
                             uint8_t statWifiErr, uint8_t statHttpErr, uint8_t statUartErr, uint8_t statWakeupCnt, int64_t statSendMS
                             );
