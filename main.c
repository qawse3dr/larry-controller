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
 *   I suggest smaller buttons for these as you dont want to take up a lot of space
 *   GAMEPAD_BUTTON_SELECT ->
 *      3v3 -> 220b -> button -> gpio10
 *   GAMEPAD_BUTTON_START ->
 *      3v3 -> 220b -> button -> gpio11
 * 4. DPAD (only 1 for now maybe ill do 2 on a second attempt)
 *   same as buttons 
 **/ 


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "pico/stdlib.h"
#include "hardware/adc.h"

#include "usb_descriptors.h"



#include "bsp/board.h"
#include "tusb.h"

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

/** Hunters Controller */
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


//todo remove
static void send_hid_report(uint8_t report_id, uint32_t btn);

struct {
  bool south_pressed;
  bool north_pressed;
  bool east_pressed;
  bool west_pressed;

  bool start_pressed;
  bool select_pressed;
  

} gamepad_status;

// todo change to false
bool controller_on = true;

/** called on mount*/
void tud_mount_cb() {
  gpio_init(SOUTH_BUTTON_PIN);
  gpio_init(WEST_BUTTON_PIN);
  gpio_init(NORTH_BUTTON_PIN);
  gpio_init(EAST_BUTTON_PIN);
  gpio_init(D_PAD_UP_PIN);
  gpio_init(D_PAD_DOWN_PIN);
  gpio_init(D_PAD_LEFT_PIN);
  gpio_init(D_PAD_RIGHT_PIN);
  gpio_init(SELECT_BUTTON_PIN);
  gpio_init(START_BUTTON_PIN);
  gpio_init(POWER_SWITCH_PIN);

  
  gpio_set_dir(SOUTH_BUTTON_PIN, GPIO_IN);
  gpio_set_dir(WEST_BUTTON_PIN, GPIO_IN);
  gpio_set_dir(NORTH_BUTTON_PIN, GPIO_IN);
  gpio_set_dir(EAST_BUTTON_PIN, GPIO_IN);
  gpio_set_dir(D_PAD_UP_PIN, GPIO_IN);
  gpio_set_dir(D_PAD_DOWN_PIN, GPIO_IN);
  gpio_set_dir(D_PAD_LEFT_PIN, GPIO_IN);
  gpio_set_dir(D_PAD_RIGHT_PIN, GPIO_IN);
  gpio_set_dir(SELECT_BUTTON_PIN, GPIO_IN);
  gpio_set_dir(START_BUTTON_PIN, GPIO_IN);
  gpio_set_dir(POWER_SWITCH_PIN, GPIO_IN);

  memset(&gamepad_status, 0, sizeof(gamepad_status));
}
// Invoked when device is unmounted
void tud_umount_cb(void){
}

void tud_suspend_cb(bool remote_wakeup_en)
{
  (void) remote_wakeup_en;

}

// Invoked when usb bus is resumed
void tud_resume_cb(void) {
  memset(&gamepad_status, 0, sizeof(gamepad_status));
}


/**
 * Polls all the buttons on the controller to see if an state is changed
 * 
 */
void hid_task() {

  // Poll every 10ms
  const uint32_t interval_ms = 10;
  static uint32_t start_ms = 0;

  if ( board_millis() - start_ms < interval_ms) return; // not enough time
  start_ms += interval_ms;

  uint32_t const btn = board_button_read();

  // Remote wakeup
  if ( tud_suspended())
  {
    // Wake up host if we are in suspend mode
    // and REMOTE_WAKEUP feature is enabled by host
    tud_remote_wakeup();
  } else {

    static bool has_gamepad_key = false;


    hid_gamepad_report_t report =
    {
      .x   = 0, .y = 0, .z = 0, .rz = 0, .rx = 0, .ry = 0,
      .hat = 0, .buttons = 0
    };

    if(gpio_get(SOUTH_BUTTON_PIN) == 1) {
    report.buttons |= GAMEPAD_BUTTON_SOUTH;
    has_gamepad_key = true;
    } 
    if(gpio_get(WEST_BUTTON_PIN) == 1) {
      report.buttons |= GAMEPAD_BUTTON_WEST;
      has_gamepad_key = true;

    } 
    if(gpio_get(NORTH_BUTTON_PIN) == 1) {
      report.buttons |= GAMEPAD_BUTTON_NORTH;
      has_gamepad_key = true;

    } 
    if(gpio_get(EAST_BUTTON_PIN) == 1) {
      report.buttons |= GAMEPAD_BUTTON_EAST;
      has_gamepad_key = true;


    } 
    if(gpio_get(SELECT_BUTTON_PIN) == 1) {
      report.buttons |= GAMEPAD_BUTTON_SELECT;
      has_gamepad_key = true;

    } 
    if(gpio_get(START_BUTTON_PIN) == 1) {
      report.buttons |= GAMEPAD_BUTTON_START;
      has_gamepad_key = true;

    }
    if(btn) {
      report.buttons |= GAMEPAD_BUTTON_START;
      has_gamepad_key = true;

    }

    int up_value = gpio_get(D_PAD_UP_PIN);
    int down_value = gpio_get(D_PAD_DOWN_PIN);
    int left_value = gpio_get(D_PAD_LEFT_PIN);
    int right_value = gpio_get(D_PAD_RIGHT_PIN);

    // get d-pad
    if (up_value == 1 && left_value == 1) {
      report.hat = GAMEPAD_HAT_UP_LEFT;
    } else if (up_value == 1 && right_value == 1) {
      report.hat = GAMEPAD_HAT_UP_RIGHT;
    } else if (up_value == 1) {
      report.hat = GAMEPAD_HAT_UP;
    } else if (down_value == 1 && left_value == 1) {
      report.hat = GAMEPAD_HAT_DOWN_LEFT;
    } else if (down_value == 1 && right_value == 1) {
      report.hat = GAMEPAD_HAT_DOWN_RIGHT;
    } else if ( down_value == 1 ) {
      report.hat = GAMEPAD_HAT_DOWN;
    } else if (left_value == 1) {
      report.hat = GAMEPAD_HAT_LEFT;
    } else if ( right_value == 1) {
      report.hat = GAMEPAD_HAT_RIGHT;
    } else {
      report.hat = GAMEPAD_HAT_CENTERED;
    }
    
    if(report.buttons == 0 && has_gamepad_key && report.hat == GAMEPAD_HAT_CENTERED) {
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
void power_task() {


}


int main() {
  adc_init();
  board_init();
  tusb_init();


  while (1) {
    tud_task();

    // check if the controller is on
    power_task();

  if ( !tud_hid_ready() ) continue;

    
    
    if(controller_on)
      hid_task();
  }
}


// Invoked when sent REPORT successfully to host
// Application can use this to send the next report
// Note: For composite reports, report[0] is report ID
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint8_t len)
{
  (void) instance;
  (void) len;


}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{

  return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {}


