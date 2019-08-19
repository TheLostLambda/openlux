#include "motors.h"

static const gpio_num_t DATA = GPIO_NUM_18;
static const gpio_num_t CLK = GPIO_NUM_19;
static const gpio_num_t LATCH = GPIO_NUM_21;

//static const int WELL_SPACING = 115;

static int TARGET_X_POSITION = 2048;
static int TARGET_Y_POSITION = -2048;
static int CURRENT_X_POSITION = 0;
static int CURRENT_Y_POSITION = 0;
static int X_STEP = 0;
static int Y_STEP = 0;
static int STEP_PERIOD = 4;

void motor_loop(void* _args) { // Maybe add step period as an arg
  while (true) {
    if(step_motors(UPPER_MOTORS, TARGET_X_POSITION - CURRENT_X_POSITION)){
      shift_byte(1 << (X_STEP + 4));
    } else {
      step_motors(LOWER_MOTORS, TARGET_Y_POSITION - CURRENT_Y_POSITION);
      shift_byte(1 << Y_STEP);
    }
    ESP_LOGI(TAG, "X: %d, Y: %d", CURRENT_X_POSITION, CURRENT_Y_POSITION);
    //shift_byte((1 << (X_STEP + 4)) + (1 << Y_STEP));
    vTaskDelay(STEP_PERIOD / portTICK_PERIOD_MS);
  }
}

// The delta is a little ugly, but just gives the direction
int step_motors(motor_set_t ms, int delta) {
  if (delta == 0) { return 0; }
  int step = (delta > 0) ? 1 : 3; // Better or worse than below?
  int true_step = (delta > 0) ? 1 : -1;
  //int step = (delta > 0) ? 1 : (delta < 0) ? 3 : 0; // This is gross
  // (x % N + N) % N
  if (ms == LOWER_MOTORS) {
    Y_STEP = (Y_STEP + step) % 4;
    CURRENT_Y_POSITION += true_step;//(1 + step) % 4 - 1;
  } else {
    // Above or below?
    //X_STEP = (X_STEP + step) % 4;
    X_STEP += step;
    X_STEP %= 4;
    CURRENT_X_POSITION += true_step;//(step == 1) ? 1 : -1;
  }
  return true_step;
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
/*void step_motors(motor_set_t ms, int delta) {
  char mask = (ms == LOWER_MOTORS) ? 0x0F : 0xF0;
  char nibble = MOTOR_BYTE & mask;
  if (delta > 0) {
    nibble = ((char)(nibble << 1) == 0) ? 0x11 : nibble << 1;
  } else {
    nibble = (nibble >> 1 == 0) ? 0x88 : nibble >> 1;
  }
  MOTOR_BYTE = (MOTOR_BYTE & ~mask) | (nibble & mask);
  }*/
    

void drive_motors(motor_set_t ms, int stp, int per) {
  char mask = (ms == LOWER_MOTORS) ? 0x0F : 0xF0;
  for (int n = 0; n < abs(stp); n++) {
    int sh = (stp > 0) ? n % 4 : 3 - n % 4;
    char byte = ((1 << (sh + 4)) + (1 << sh)) & mask;
    vTaskDelay(per / portTICK_PERIOD_MS);
    shift_byte(byte);
  }
}
