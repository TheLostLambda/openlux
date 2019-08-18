#include "sensors.h"
#include "common.h"
#include <math.h>

typedef struct poll_args {
  adc1_channel_t channel;
  unsigned int samples;
} poll_args_t;

static int SENSOR_VALUE = -1;
static void poll_avg(void*);

// Return the task handle
void start_sensor_polling(adc1_channel_t ch, adc_atten_t atn, unsigned int per) {
  die_politely(adc1_config_width(ADC_WIDTH_BIT_12), "Failed to set ADC bus width");
  die_politely(adc1_config_channel_atten(ch, atn), "Failed to set channel attenuation");
  ESP_LOGI(TAG, "Started sensor polling on channel %d at %.2fHz", ch, 1000/((double) per));
  poll_args_t* args = (poll_args_t*) malloc(sizeof(poll_args_t));
  args->channel = ch;
  args->samples = per / (unsigned int) portTICK_PERIOD_MS;
  TaskHandle_t pollHandle = NULL;
  xTaskCreate(poll_avg, "SENSOR_POLLING", 4096, args, 3, &pollHandle);
}

int get_sensor_value(void) {
  return SENSOR_VALUE;
}
  
static void poll_avg(void* args) {
  poll_args_t* opts = (poll_args_t*)args;
  while (true) {
    float avg = 0;
    for (int i = 0; i < opts->samples; i++) {
      avg += adc1_get_raw(opts->channel);
      vTaskDelay(1);
    }
    SENSOR_VALUE = round(avg / opts->samples);
  }
}
