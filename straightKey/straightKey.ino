
// ============================================================
// A Morse code straight key practice oscillator
//
// (c) Scott Baker KJ7NLA
// ============================================================

#include <Arduino.h>
#include <Wire.h>

#define BAUDRATE 115200
#define BUFDEPTH 512

const int pinKey  = 2;      // straight key input
const int pinLed  = 4;      // LED pin
const int pinBuzz = 8;      // buzzer/speaker pin

const int debugOn = 1;      // print debug messages
const int ddHz    = 659;    // beep @E5 dit/dah

// Arduino init
//
void setup() {
  pinMode(pinKey,  INPUT_PULLUP);
  pinMode(pinLed,  OUTPUT);
  pinMode(pinBuzz, OUTPUT);
  Serial.begin(BAUDRATE);
}

// Arduino main loop
//
void loop() {
  if (!digitalRead(pinKey)) {
    digitalWrite(pinLed, HIGH);
    tone(pinBuzz, ddHz);
  } else {
    digitalWrite(pinLed, LOW);
    noTone(pinBuzz);
  }
  delayMicroseconds(150);
}

