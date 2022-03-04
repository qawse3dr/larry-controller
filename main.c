/**
 * @author qawse3dr a.k.a Larry Milne
 * This is my first attempt at programming a hid custom controller
 * a lot of the code is borrowed from the examples of tiny usb/ pico's git page
 * https://github.com/hathach/tinyusb/tree/master/examples/device/hid_composite
 */

/**
 * Design of the board
 *
 * NOTE: The board must fit this description of it to work
 *
 * 1.  3v3->  1k resistor ->  led (to indicate its on) -> ground
 * 2. 4 face buttons (a, b, x, y || south, east, west, north)
 *    for these i suggest bigger buttons colours ones are also fun!
 *   GAMEPAD_BUTTON_SOUTH
 *      3v3 ->  220b resistor -> button -> gpio0
 *  GAMEPAD_BUTTON_WEST
 *      3v3 ->  220b resistor -> button -> gpio1
 *  GAMEPAD_BUTTON_NORTH
 *      3v3 ->  220b resistor -> button -> gpio2
 *  GAMEPAD_BUTTON_EAST
 *      3v3 ->  220b resistor -> button -> gpio3
 *
 * 3. select and start buttons
 *   I suggest smaller buttons for these as you dont want to take up a lot of
 *space GAMEPAD_BUTTON_SELECT -> 3v3 -> 220b -> button -> gpio10
 *   GAMEPAD_BUTTON_START ->
 *      3v3 -> 220b -> button -> gpio11
 * 4. DPAD (only 1 for now maybe ill do 2 on a second attempt)
 *   same as buttons
 **/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bsp/board.h"
#include "mcp3008.h"
#include "pico/stdlib.h"
#include "tusb.h"
#include "usb_descriptors.h"

/** Larrys Controller
#define SOUTH_BUTTON_PIN 2
#define WEST_BUTTON_PIN 5
#define NORTH_BUTTON_PIN 4
#define EAST_BUTTON_PIN 3

#define SELECT_BUTTON_PIN 11
#define START_BUTTON_PIN 10

#define POWER_SWITCH_PIN 16

#define D_PAD_UP_PIN 22
#define D_PAD_DOWN_PIN 19
#define D_PAD_LEFT_PIN 20
#define D_PAD_RIGHT_PIN 21
*/

/** Hunters Controller
#define SOUTH_BUTTON_PIN 11
#define WEST_BUTTON_PIN 4
#define NORTH_BUTTON_PIN 5
#define EAST_BUTTON_PIN 12

#define SELECT_BUTTON_PIN 21
#define START_BUTTON_PIN 13

#define POWER_SWITCH_PIN 16

#define D_PAD_UP_PIN 22
#define D_PAD_DOWN_PIN 19
#define D_PAD_LEFT_PIN 18
#define D_PAD_RIGHT_PIN 20
*/

/** Larrys v2 Controller */
#define SOUTH_BUTTON_PIN 22
#define WEST_BUTTON_PIN 20
#define NORTH_BUTTON_PIN 27
#define EAST_BUTTON_PIN 21

#define SELECT_BUTTON_PIN 13
#define START_BUTTON_PIN 19
#define SPECIAL_BUTTON_1_PIN 9
#define SPECIAL_BUTTON_2_PIN 26

#define LEFT_BUMPER_PIN 1
#define RIGHT_BUMPER_PIN 0

#define JOYSTICK_LEFT_Z 14
#define JOYSTICK_RIGHT_Z 16
#define D_PAD_UP_PIN 6
#define D_PAD_DOWN_PIN 10
#define D_PAD_LEFT_PIN 8
#define D_PAD_RIGHT_PIN 12

static void send_hid_report(uint8_t report_id, uint32_t btn);

// todo change to false
bool controller_on = true;

static struct MCP3008* mcp3008 = NULL;
/** called on mount*/
void tud_mount_cb() {
  static int pins[] = {
      SOUTH_BUTTON_PIN,     WEST_BUTTON_PIN,      NORTH_BUTTON_PIN,
      EAST_BUTTON_PIN,      D_PAD_UP_PIN,         D_PAD_DOWN_PIN,
      D_PAD_LEFT_PIN,       D_PAD_RIGHT_PIN,      SELECT_BUTTON_PIN,
      START_BUTTON_PIN,     LEFT_BUMPER_PIN,      RIGHT_BUMPER_PIN,
      SPECIAL_BUTTON_1_PIN, SPECIAL_BUTTON_2_PIN, JOYSTICK_LEFT_Z,
      JOYSTICK_RIGHT_Z};

  for (int i = 0; i < sizeof(pins) / sizeof(int); i++) {
    gpio_init(pins[i]);
    gpio_set_dir(pins[i], GPIO_IN);
  }

  // Joysticks are pull up
  gpio_pull_up(JOYSTICK_LEFT_Z);
  gpio_pull_up(JOYSTICK_RIGHT_Z);

  // Create spi device
  mcp3008 = init_mcp3008(0, 2, 3, 4, 5, 10000);
}
// Invoked when device is unmounted
void tud_umount_cb(void) {}

void tud_suspend_cb(bool remote_wakeup_en) { (void)remote_wakeup_en; }

// Invoked when usb bus is resumed
void tud_resume_cb(void) {}

/**
 * Polls all the buttons on the controller to see if an state is changed
 *
 */
void hid_task() {
  // Poll every 10ms
  const uint32_t interval_ms = 10;
  static uint32_t start_ms = 0;

  if (board_millis() - start_ms < interval_ms) return;  // not enough time
  start_ms += interval_ms;

  // Remote wakeup
  if (tud_suspended()) {
    // Wake up host if we are in suspend mode
    // and REMOTE_WAKEUP feature is enabled by host
    tud_remote_wakeup();
  } else {
    static bool has_gamepad_key = false;
    static int buttons_pins[] = {
        SOUTH_BUTTON_PIN,     WEST_BUTTON_PIN,   NORTH_BUTTON_PIN,
        EAST_BUTTON_PIN,      SELECT_BUTTON_PIN, START_BUTTON_PIN,
        LEFT_BUMPER_PIN,      RIGHT_BUMPER_PIN,  SPECIAL_BUTTON_1_PIN,
        SPECIAL_BUTTON_2_PIN, JOYSTICK_LEFT_Z,   JOYSTICK_RIGHT_Z};
    static int buttons_pins_values[] = {
        GAMEPAD_BUTTON_SOUTH, GAMEPAD_BUTTON_WEST,   GAMEPAD_BUTTON_NORTH,
        GAMEPAD_BUTTON_EAST,  GAMEPAD_BUTTON_SELECT, GAMEPAD_BUTTON_START,
        GAMEPAD_BUTTON_TL,    GAMEPAD_BUTTON_TR,     GAMEPAD_BUTTON_MODE,
        GAMEPAD_BUTTON_C,     GAMEPAD_BUTTON_THUMBL, GAMEPAD_BUTTON_THUMBR};
    hid_gamepad_report_t report = {.x = 0,
                                   .y = 0,
                                   .z = 0,
                                   .rz = 0,
                                   .rx = 0,
                                   .ry = 0,
                                   .hat = 0,
                                   .buttons = 0};

    // get buttons value
    for (int i = 0; i < 10 /*sizeof(button_pins)/sizeof(int)*/; i++) {
      if (gpio_get(buttons_pins[i]) == 1) {
        report.buttons |= buttons_pins_values[i];
        has_gamepad_key = true;
      }
    }
    if (gpio_get(buttons_pins[10]) == 0) {
      report.buttons |= buttons_pins_values[10];
      has_gamepad_key = true;
    }

    if (gpio_get(buttons_pins[11]) == 0) {
      report.buttons |= buttons_pins_values[11];
      has_gamepad_key = true;
    }

    // I could probably use a hash table here but
    // I am feeling pretty lazy so IF STATEMENTS!!!!
    // get dpad value
    int up_value = gpio_get(D_PAD_UP_PIN);
    int down_value = gpio_get(D_PAD_DOWN_PIN);
    int left_value = gpio_get(D_PAD_LEFT_PIN);
    int right_value = gpio_get(D_PAD_RIGHT_PIN);

    if (up_value) {
      if (left_value) {
        report.hat = GAMEPAD_HAT_UP_LEFT;
      } else if (right_value) {
        report.hat = GAMEPAD_HAT_UP_RIGHT;
      } else {
        report.hat = GAMEPAD_HAT_UP;
      }
    } else if (down_value) {
      if (left_value) {
        report.hat = GAMEPAD_HAT_DOWN_LEFT;
      } else if (right_value) {
        report.hat = GAMEPAD_HAT_DOWN_RIGHT;
      } else {
        report.hat = GAMEPAD_HAT_DOWN;
      }
    } else if (left_value) {
      report.hat = GAMEPAD_HAT_LEFT;
    } else if (right_value) {
      report.hat = GAMEPAD_HAT_RIGHT;
    } else {
      report.hat = GAMEPAD_HAT_CENTERED;
    }

    // joystick  Reads in 1024 with middle being 512 thus subtract 512 to get
    // real value
    report.rx = (mcp3008->read(mcp3008, 1) - 512) / 4;
    report.ry = (mcp3008->read(mcp3008, 0) - 512) / 4;
    report.x = (mcp3008->read(mcp3008, 6) - 512) / 4;
    report.y = (mcp3008->read(mcp3008, 7) - 512) / 4;

    if (report.buttons == 0 && has_gamepad_key &&
        report.hat == GAMEPAD_HAT_CENTERED) {
      tud_hid_report(REPORT_ID_GAMEPAD, &report, sizeof(report));
      has_gamepad_key = false;
    } else {
      tud_hid_report(REPORT_ID_GAMEPAD, &report, sizeof(report));
    }
  }
}

/**
 * Polls to see if the controller is turned on from the switch
 */
void power_task() {}

int main() {
  board_init();
  tusb_init();

  while (1) {
    tud_task();

    // check if the controller is on
    power_task();

    if (!tud_hid_ready()) continue;

    if (controller_on) hid_task();
  }
}

// Invoked when sent REPORT successfully to host
// Application can use this to send the next report
// Note: For composite reports, report[0] is report ID
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report,
                                uint8_t len) {
  (void)instance;
  (void)len;
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                               hid_report_type_t report_type, uint8_t* buffer,
                               uint16_t reqlen) {
  return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                           hid_report_type_t report_type, uint8_t const* buffer,
                           uint16_t bufsize) {}
