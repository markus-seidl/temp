#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "soc/adc_channel.h"
#include "esp_log.h"

#define NO_OF_SAMPLES   32          //Multisampling
#define ADC_ATTEN       ADC_ATTEN_DB_6
#define ADC_GPIO        ADC1_GPIO1_CHANNEL
#define ADC_WIDTH       ADC_WIDTH_BIT_13

static const char *TAG = "adc";

static void print_char_val_type(esp_adc_cal_value_t val_type)
{
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        ESP_LOGD(TAG, "Characterized using Two Point Value");
    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        ESP_LOGD(TAG, "Characterized using eFuse Vref");
    } else {
        ESP_LOGD(TAG, "Characterized using Default Vref");
    }
}


uint32_t adc_read_voltage(void) {
    // Configure ADC
    adc1_config_width(ADC_WIDTH);
    adc1_config_channel_atten(ADC_GPIO, ADC_ATTEN);

    // Characterize ADC

    esp_adc_cal_characteristics_t *adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN, ADC_WIDTH,
                                                            -1 /* unused on esp32s2 */, adc_chars
                                                            );
    print_char_val_type(val_type);

    // Sample ADC1
    uint32_t adc_reading = 0;

    for (int i = 0; i < NO_OF_SAMPLES; i++) { // Multisampling
        adc_reading += adc1_get_raw((adc1_channel_t)ADC_GPIO);
    }
    adc_reading /= NO_OF_SAMPLES;

    //Convert adc_reading to voltage in mV
    uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
    voltage *= 3;
    ESP_LOGI(TAG, "Raw: %d\tVoltage: %dmV\n", adc_reading, voltage);

    free(adc_chars);

    return voltage;
}
