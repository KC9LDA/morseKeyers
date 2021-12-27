
// ============================================================
// A Vband Morse code keyer to USB interface
//
// Hardware = AdaFruit Trinket M0 + Morse code paddle
// connect dit paddle to Trinket pin 3
// connect dah paddle to Trinket pin 0
//
// Connect to Vband @ https://hamradio.solutions/vband/
//
// (c) Scott Baker KJ7NLA
// ============================================================

#include <Adafruit_DotStar.h>
#include <Keyboard.h>

#define pinDah 0
#define pinDit 3

#define DIT_KEY   KEY_LEFT_CTRL
#define DAH_KEY   KEY_RIGHT_CTRL

uint8_t dit_pressed = 0;
uint8_t dah_pressed = 0;
uint8_t dit_sent = 0;
uint8_t dah_sent = 0;

Adafruit_DotStar strip = Adafruit_DotStar(1, INTERNAL_DS_DATA, INTERNAL_DS_CLK, DOTSTAR_BGR);

void getPaddles(void);

// read and debounce paddles
void getPaddles() {

  int ditV1 = !digitalRead(pinDit);
  int dahV1 = !digitalRead(pinDah);
  delay(10);
  int ditV2 = !digitalRead(pinDit);
  int dahV2 = !digitalRead(pinDah);

  if (!dit_pressed && ditV1 && ditV2) {
    dit_pressed = 1;
  }
  if (!dah_pressed && dahV1 && dahV2) {
    dah_pressed = 1;
  }
  if (dit_pressed && !ditV1 && !ditV2) {
    dit_pressed  = 0;
  }
  if (dah_pressed && !dahV1 && !dahV2) {
    dah_pressed  = 0;
  }

}

void setup() {
  pinMode(pinDit, INPUT_PULLUP);
  pinMode(pinDah, INPUT_PULLUP);
  Keyboard.begin();  // init USB keyboard
  strip.begin();     // init DotStar LED
  strip.setPixelColor(0, 0x000000); // off
  strip.show();
}

void loop() {
  getPaddles();

  // send key pressed to USB
  if (dit_pressed) {
    Keyboard.press(DIT_KEY);
    dit_sent = 1;
  }
  if ( dah_pressed) {
    Keyboard.press(DAH_KEY);
    dah_sent = 1;
  }

  // send key released to USB
  if (dit_sent && !dit_pressed) {
    Keyboard.release(DIT_KEY);
    dit_sent = 0;
  }
  if (dah_sent && !dah_pressed) {
    Keyboard.release(DAH_KEY);
    dah_sent = 0;
  }
}

