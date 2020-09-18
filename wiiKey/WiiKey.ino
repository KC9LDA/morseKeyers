
// ============================================================
// A Morse Code Iambic Keyer using a Wii Nunchuk
//
// (c) Scott Baker KJ7NLA
// ============================================================

#include <Arduino.h>
#include <Wire.h>

#define BAUDRATE 115200
#define ADDRESS  0x52

const int pinLed  = 4;      // LED pin
const int pinBuzz = 8;      // buzzer/speaker pin

const int debugOn = 0;      // print debug messages

const int minWpm  = 10;     // min speed
const int midWpm  = 15;     // initial speed
const int maxWpm  = 25;     // max speed

const int maxJs   = 180;    // analog joystick max
const int minJs   = 80;     // analog joystick min

const int skHz    = 220;    // beep @A4 swap keys
const int dkHz    = 262;    // beep @C4 default keys
const int dnHz    = 440;    // beep @A5 WPM-
const int upHz    = 523;    // beep @C5 WPM+
const int ddHz    = 659;    // beep @E5 dit/dah

const int beepLen = 69;     // system beep  length
const int beepDly = 99;     // system delay length

int letterDone    = 0;      // letter complete
int wordDone      = 0;      // word   complete
int wasDit        = 0;      // prev state was dit
int swapKeys      = 0;      // swap dit/dah keys

int actualWpm;              // words per minute
int ditLength;              // dit length in ms
int ditPaddle;              // dit paddle state
int dahPaddle;              // dah paddle state
int Xaxis;                  // joystick X axis
int Yaxis;                  // joystick Y axis


// program setup
void setup() {
  pinMode(pinLed,  OUTPUT);
  pinMode(pinBuzz, OUTPUT);
  nunchuk_init();
  initWpm();
  Serial.begin(BAUDRATE);
}

// main loop
void loop() {

  nunchuk_update();

  // check for joystick input
  if ((Yaxis > maxJs) || (Yaxis < minJs) ||
      (Xaxis > maxJs) || (Xaxis < minJs)) {

    if ((Yaxis > maxJs) || (Yaxis < minJs)) {
      // update WPM
      updateWpm();
    }
    if ((Xaxis > maxJs) || (Xaxis < minJs)) {
      // update key bindings
      updateKeys();
    }
  } else {
    if (ditPaddle && dahPaddle) {
      // iambic mode
      letterDone = wordDone = 0;
      if (wasDit) {
        doDah();
      } else {
        doDit();
      }
    } else if (ditPaddle) {
      // send a dit
      letterDone = wordDone = 0;
      doDit();
    } else if (dahPaddle) {
      // send a dah
      letterDone = wordDone = 0;
      doDah();
    } else {
      // check if letter complete
      if (!letterDone){
        letterDone = 1;
        letterGap();
      } else {
        // check if word complete
        if (!wordDone){
          wordDone = 1;
          wordGap();
        }
      }
    }
  }
}

// initial WPM
void initWpm() {
  actualWpm = midWpm;
  getDitLength();
}

// dit/dah key swapping
void updateKeys() {
  if ((Xaxis < minJs) && !swapKeys) {
    swapKeys = 1;
    printc2("dit/dah are swapped");
    beep(skHz);
  }
  if ((Xaxis > maxJs) && swapKeys) {
    swapKeys = 0;
    printc2("dit/dah are unswapped");
    beep(dkHz);
  }
}

// increase or decrease WPM
void updateWpm() {
  if (Yaxis > maxJs) {
    actualWpm++;
    if (actualWpm > maxWpm) {
      actualWpm = maxWpm;
      return;
    }
    beep(upHz);
  } else {
    actualWpm--;
    if (actualWpm < minWpm) {
      actualWpm = minWpm;
      return;
    }
    beep(dnHz);
  }
  getDitLength();
  // read the Wii nunchuck status
  nunchuk_update();
  ditPaddle = 0;
  dahPaddle = 0;
  printWPM();
}

// Calculate dit length in ms
void getDitLength() {
  ditLength = 1200 / actualWpm;
}

// play dit
void doDit() {
  printc(".");
  digitalWrite(pinLed, HIGH);
  wasDit = 1;
  tone(pinBuzz, ddHz);
  delay(ditLength);
  noTone(pinBuzz);
  digitalWrite(pinLed, LOW);
  delay(ditLength);
}

// play dah
void doDah() {
  printc("-");
  digitalWrite(pinLed, HIGH);
  wasDit = 0;
  tone(pinBuzz, ddHz);
  delay(ditLength*3);
  noTone(pinBuzz);
  digitalWrite(pinLed, LOW);
  delay(ditLength);
}

// delay by letter gap
void letterGap() {
  printc(" ");
  delay(ditLength*2);
}

// delay by word gap
void wordGap() {
  printc("/ ");
  delay(ditLength*4);
}

// play system beep
void beep(int freq) {
  tone(pinBuzz, freq);
  delay(beepLen);
  noTone(pinBuzz);
  delay(beepDly);
}

// debug print
void printWPM() {
  if (debugOn) {
    Serial.print("Setting WPM to ");
    Serial.println(actualWpm);
  }
}

// debug print
void printc2(char *str) {
  if (debugOn) {
    Serial.println(str);
  }
}

// debug print
void printc(char *str) {
  if (debugOn) {
    Serial.print(str);
  }
}

// ===========================
// nunchuk routines
// ===========================

void nunchuk_init() {
  Wire.begin();
  sendByte(0x55, 0xF0);
  sendByte(0x00, 0xFB);
  nunchuk_update();
}

void nunchuk_update() {
  int count = 0;
  int values[6];
  int zButton;
  int cButton;

  Wire.requestFrom(ADDRESS, 6);

  while(Wire.available()) {
    values[count] = Wire.read();
    count++;
  }

  Xaxis = values[0];
  Yaxis = values[1];
  zButton = !((values[5] >> 0) & 1);
  cButton = !((values[5] >> 1) & 1);
  sendByte(0x00, 0x00);

  // get dit/dah paddle status
  if (swapKeys) {
    ditPaddle = cButton;
    dahPaddle = zButton;
  } else {
    ditPaddle = zButton;
    dahPaddle = cButton;
  }
}

void sendByte(byte data, byte location) {
  Wire.beginTransmission(ADDRESS);
  Wire.write(location);
  Wire.write(data);
  Wire.endTransmission();
  delay(2);
}

