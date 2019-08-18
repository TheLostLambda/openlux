#include "motors.h"

static const gpio_num_t DATA = GPIO_NUM_16;
static const gpio_num_t CLK = GPIO_NUM_17;
static const gpio_num_t LATCH = GPIO_NUM_18;

static int TARGET_X_POSITION = 0;
static int TARGET_Y_POSITION = 0;
static int CURRENT_X_POSITION = 0;
static int CURRENT_Y_POSITION = 0;

static char MOTOR_BYTE = 0x84;
static int STEP_PERIOD = 2;

void motor_loop(void* _args) { // Maybe add step period as an arg
  while (true) {
    shift_byte(MOTOR_BYTE);
    vTaskDelay(STEP_PERIOD / portTICK_PERIOD_MS);
  }
}

// Kill me later
int get_motor_byte() {
  return MOTOR_BYTE;
}

void setup_motor_driver() {
  // Error handle me...
  gpio_pad_select_gpio(DATA);
  gpio_pad_select_gpio(CLK);
  gpio_pad_select_gpio(LATCH);
  gpio_set_direction(DATA, GPIO_MODE_OUTPUT);
  gpio_set_direction(CLK, GPIO_MODE_OUTPUT);
  gpio_set_direction(LATCH, GPIO_MODE_OUTPUT);
  TaskHandle_t motorHandle = NULL;
  xTaskCreate(motor_loop, "MOTOR_LOOP", 4096, NULL, 3, &motorHandle);
}

void shift_byte(char byte) {
  gpio_set_level(LATCH, 0);
  for (int n = 0; n < 8; n++) {
    // Get the nth bit
    uint32_t bit = ((byte & (1 << n)) == 0) ? 0 : 1;
    gpio_set_level(DATA, bit);
    gpio_set_level(CLK, 1);
    gpio_set_level(CLK, 0);
  }
  gpio_set_level(LATCH, 1);
}

// The delta is a little ugly, but just gives the direction
void step_motors(motor_set_t ms, int delta) {
  char mask = (ms == LOWER_MOTORS) ? 0x0F : 0xF0;
  char nibble = MOTOR_BYTE & mask;
  if (delta > 0) {
    nibble = ((char)(nibble << 1) == 0) ? 0x11 : nibble << 1;
  } else {
    nibble = (nibble >> 1 == 0) ? 0x88 : nibble >> 1;
  }
  MOTOR_BYTE = (MOTOR_BYTE & ~mask) | (nibble & mask);
}
    

void drive_motors(motor_set_t ms, int stp, int per) {
  char mask = (ms == LOWER_MOTORS) ? 0x0F : 0xF0;
  for (int n = 0; n < abs(stp); n++) {
    int sh = (stp > 0) ? n % 4 : 3 - n % 4;
    char byte = ((1 << (sh + 4)) + (1 << sh)) & mask;
    vTaskDelay(per / portTICK_PERIOD_MS);
    shift_byte(byte);
  }
}
