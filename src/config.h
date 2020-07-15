#pragma once

#define SSID "comet compat"
#define PASS "logicalis"

/* Number of LEDs in the led strip. If the strip is longer than this, the last
 * few LEDs will float. If the strip is shorter than this, there are no visual
 * consequences. Controls the size of internal buffers and the length of many
 * loops. */
#define NUM_LEDS 16

/* hardware gpio pins for bitbanging, in Arduino notation NOT NODEMCU NOTATION
 *
 * \/\/\/\/
 *  |,,,,|
 *  |    |
 *  | [] |  /\
 *  |,,,,|  ||
 *  |    |  ||
 *  | [] |
 *  \,,,,/
 *   ||||_____ Ground
 *   |||
 *   |||_____ CLK_PIN
 *   ||
 *   ||_____ DATA_PIN
 *   |
 *   |______ 5v DC in
 */
#define DATA_PIN 13
#define CLK_PIN 14

// number of seconds until we give up on connecting to the predefined AP and
// create our own
#define WIFI_GIVEUP 10

// hardware framerate for all animations
#define ANIM_HZ 60
