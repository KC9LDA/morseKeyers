
// ============================================================
// A Morse Code Iambic Keyer
//
// (c) Scott Baker KJ7NLA
// ============================================================

#include <Arduino.h>
#include <Wire.h>

#define BAUDRATE 115200
#define BUFDEPTH 32

const int pinDit  = 2;      // dit key input
const int pinDah  = 3;      // dah key input
const int pinLed  = 5;      // LED pin (to relay)
const int pinSw1  = 7;      // push-button switch
const int pinBuzz = 8;      // buzzer/speaker pin

const int debugOn = 0;      // print debug messages

const int ditdahMode = 0;   // normal/dit-dah mode
const int configMode = 1;   // configuration mode
const int recallMode = 2;   // recall message mode

const int iambicA    = 0;   // iambic key mode A
const int iambicB    = 1;   // iambic key mode B
const int straight   = 2;   // straight key mode

const int minWpm  = 10;     // min speed
const int midWpm  = 18;     // initial speed
const int maxWpm  = 25;     // max speed

const int skHz    = 220;    // beep @A4 at WPM limit
const int dnHz    = 440;    // beep @A5 WPM x10
const int upHz    = 523;    // beep @C5 WPM x1
const int ddHz    = 659;    // beep @E5 dit/dah

const int beepLen = 69;     // system beep  length
const int beepDly = 99;     // system delay length
const int swTmax  = 100;    // switch 1 timeout value

const char *msg1  = "CQ CQ DE XXXXX XXXXX KN";
const char *msg2  = "1234567890";
const char *msg3  = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

char mcode[8];              // morse code buffer

int cmode         = 0;      // normal/dit-dah mode
int kmode         = 0;      // iambic key mode A
int newCmode      = 0;      // cmode has changed
int newKmode      = 0;      // kmode has changed
int newWPM        = 0;      // WPM has changed
int letterDone    = 0;      // letter complete
int wordDone      = 0;      // word   complete
int wasDit        = 0;      // prev state was dit
int swapKeys      = 0;      // swap dit/dah keys

int actualWpm;              // words per minute
int ditLength;              // dit length in ms
int ditPaddle;              // dit paddle state
int dahPaddle;              // dah paddle state
int sw1Pushed;              // push-button state
int sw1Timer;               // switch-press time
int sw2Timer;               // switch-press time

void sendMorseMsg(char *msg);
void sendMorseChar(char acode);
void morseLookup(char acode);
void initWpm(void);
void updateWpm(void);
void getDitLength(void);
void readPaddles(void);
void countSw1(void);
void recallMsg(void);
void doStright(void);
void doDitDah(void);
void doDit(void);
void doDah(void);
void letterGap(void);
void wordGap(void);
void myDelay(int dly);
void beepWPM(void);
void beepN(int freq, int N);
void beep(int freq);
void printWPM();
void printc(char *str);
void doError(void);

void(*resetFunc) (void) = 0;  // reset function at address 0

// program setup
void setup() {
  pinMode(pinDit,  INPUT_PULLUP);
  pinMode(pinDah,  INPUT_PULLUP);
  pinMode(pinSw1,  INPUT_PULLUP);
  pinMode(pinLed,  OUTPUT);
  pinMode(pinBuzz, OUTPUT);
  sw1Timer = 0;
  sw2Timer = 0;
  cmode = ditdahMode;
  kmode = iambicA;
  Serial.begin(BAUDRATE);
  initWpm();
}

// handle error
//
void doError() {
  resetFunc();   // reset the Arduino
}

// main loop
void loop() {
  readPaddles();
  if (ditPaddle || dahPaddle) {
    switch (cmode) {
      case ditdahMode:
        if (kmode == straight) {
          doStright();
        } else {
          doDitDah();
        }
        break;
      case configMode:
        updateWpm();
        break;
      case recallMode:
        recallMsg();
        break;
      default:
        doError();
    }
  }
  if (sw1Pushed) {
    countSw1();
  }
}

// send a Morse code message
//
void sendMorseMsg(char *msg) {
  int  i = 0;
  char c = 1;
  while (c) {
    c = msg[i++];
    if (c) {sendMorseChar(c);}
  }
}

// send the Morse code for a letter
//
void sendMorseChar(char acode) {
  int  len;        // length of code
  int  i;          // counter
  morseLookup(acode);
  len = strlen(mcode);
  if (len > 0) {
    for (i=0; i < len; i++) {
      switch (mcode[i]) {
        case 'x':  // word boundary
          wordGap();
          break;
        case '*':  // send dit
          doDit();
          break;
        case '-':  // send dah
          doDah();
          break;
        default:
          letterGap();
      }
    }
    letterGap();
  }
}

// lookup the Morse code
//
void morseLookup(char acode) {
  if ((acode > 96) && (acode < 123)) {
    // convert to upper case
    acode -= 32;
  }
  switch (acode) {
    case 65:  // A  
      strcpy(mcode, "*-");
      break;
    case 66:  // B  
      strcpy(mcode, "-***");
      break;
    case 67:  // C  
      strcpy(mcode, "-*-*");
      break;
    case 68:  // D  
      strcpy(mcode, "-**");
      break;
    case 69:  // E  
      strcpy(mcode, "*");
      break;
    case 70:  // F  
      strcpy(mcode, "**-*");
      break;
    case 71:  // G  
      strcpy(mcode, "--*");
      break;
    case 72:  // H  
      strcpy(mcode, "****");
      break;
    case 73:  // I  
      strcpy(mcode, "**");
      break;
    case 74:  // J  
      strcpy(mcode, "*---");
      break;
    case 75:  // K  
      strcpy(mcode, "-*-");
      break;
    case 76:  // L  
      strcpy(mcode, "*-**");
      break;
    case 77:  // M  
      strcpy(mcode, "--");
      break;
    case 78:  // N  
      strcpy(mcode, "-*");
      break;
    case 79:  // O  
      strcpy(mcode, "---");
      break;
    case 80:  // P  
      strcpy(mcode, "*--*");
      break;
    case 81:  // Q  
      strcpy(mcode, "--*-");
      break;
    case 82:  // R  
      strcpy(mcode, "*-*");
      break;
    case 83:  // S  
      strcpy(mcode, "***");
      break;
    case 84:  // T  
      strcpy(mcode, "-");
      break;
    case 85:  // U  
      strcpy(mcode, "**-");
      break;
    case 86:  // V  
      strcpy(mcode, "***-");
      break;
    case 87:  // W  
      strcpy(mcode, "*--");
      break;
    case 88:  // X  
      strcpy(mcode, "-**-");
      break;
    case 89:  // Y  
      strcpy(mcode, "-*--");
      break;
    case 90:  // Z  
      strcpy(mcode, "--**");
      break;
    case 48:  //  0
      strcpy(mcode, "-----");
      break;
    case 49:  //  1
      strcpy(mcode, "*----");
      break;
    case 50:  //  2
      strcpy(mcode, "**---");
      break;
    case 51:  //  3
      strcpy(mcode, "***--");
      break;
    case 52:  //  4
      strcpy(mcode, "****-");
      break;
    case 53:  //  5
      strcpy(mcode, "*****");
      break;
    case 54:  //  6
      strcpy(mcode, "-****");
      break;
    case 55:  //  7
      strcpy(mcode, "--***");
      break;
    case 56:  //  8
      strcpy(mcode, "---**");
      break;
    case 57:  //  9
      strcpy(mcode, "----*");
      break;
    case 32:  // Space (word boudary)
      strcpy(mcode, "x");
      break;
    case 34:  // Quotation Marks
      strcpy(mcode, "*-**-*");
      break;
    case 39:  // Apostrophe
      strcpy(mcode, "*----*");
      break;
    case 40:  // Parentheses
      strcpy(mcode, "-*--*-");
      break;
    case 41:  // Parentheses
      strcpy(mcode, "-*--*-");
      break;
    case 44:  // Comma
      strcpy(mcode, "--**--");
      break;
    case 46:  // Period
      strcpy(mcode, "*-*-*-");
      break;
    case 47:  // Fraction Bar
      strcpy(mcode, "-**-*");
      break;
    case 58:  // Colon
      strcpy(mcode, "---***");
      break;
    case 63:  // Question Mark
      strcpy(mcode, "**--**");
      break;
    case 45:  // Hyphen
      strcpy(mcode, "-****-");
      break;
    default:
      strcpy(mcode, "");
  }
}

// initial WPM
void initWpm() {
  actualWpm = midWpm;
  getDitLength();
}

// increase or decrease WPM
void updateWpm() {
  int x;
  // nothing to do if no paddle is pressed
  if (!ditPaddle && !dahPaddle) {
    return;
  }
  // at this point we know the WPM will change
  newWPM = 1;
  // delay for simultaneous paddle check
  myDelay(100);
  x = ditPaddle + (dahPaddle<<1);
  switch (x) {
    case 1:
      // press dit paddle to increase WPM
      actualWpm++;
      if (actualWpm > maxWpm) {
        actualWpm = maxWpm;
        Serial.println("At max WPM = 25");
        beepN(skHz, 2);
        delay(500);
      } else {
        beep(upHz);
      }
      break;
    case 2:
      // press dah paddle to decrease WPM
      actualWpm--;
      if (actualWpm < minWpm) {
        actualWpm = minWpm;
        Serial.println("At min WPM = 10");
        beepN(skHz, 2);
        delay(500);
      } else {
        beep(dnHz);
      }
      break;
    case 3:
      // press both paddles to announce WPM
      beepWPM();
      break;
    default:
      // no paddle is pressed
      break;
  }
  getDitLength();
  printWPM();
}

// Calculate dit length in ms
void getDitLength() {
  ditLength = 1200 / actualWpm;
}

// read and debounce paddles
void readPaddles() {
  int ditV1 = !digitalRead(pinDit);
  int dahV1 = !digitalRead(pinDah);
  int sw1V1 = !digitalRead(pinSw1);
  delay(10);  // delay 10 ms
  int ditV2 = !digitalRead(pinDit);
  int dahV2 = !digitalRead(pinDah);
  int sw1V2 = !digitalRead(pinSw1);
  if (ditV1 && ditV2) {
    ditPaddle = 1;
  }
  if (!(ditV1 || ditV2)) {
    ditPaddle = 0;
  }
  if (dahV1 && dahV2) {
    dahPaddle = 1;
  }
  if (!(dahV1 || dahV2)) {
    dahPaddle = 0;
  }
  if (sw1V1 && sw1V2) {
    sw1Pushed = 1;
  }
  if (!(sw1V1 || sw1V2)) {
    sw1Pushed = 0;
  }
}

void countSw1() {
  // while sw1 is pushed
  while (sw1Pushed) {
    readPaddles();
    if (ditPaddle) {
      // cycle through iambic/straight key modes
      sw1Timer = 0;
      sw2Timer++;
      if (sw2Timer > swTmax) {
        newCmode = 0;
        newKmode = 1;
        switch (kmode) {
          case iambicA:
            kmode = iambicB;
            sw2Timer = 0;
            beepN(ddHz, 2);
            myDelay(500);
            break;
          case iambicB:
            kmode = straight;
            sw2Timer = 0;
            beepN(ddHz, 3);
            myDelay(500);
            break;
          case straight:
            kmode = iambicA;
            sw2Timer = 0;
            beepN(ddHz, 1);
            myDelay(500);
            break;
          default:
            doError();
        }
      }
    } else {
      // cycle through send/config/message modes
      sw1Timer++;
      if (sw1Timer > swTmax) {
        newCmode = 1;
        switch (cmode) {
          case ditdahMode:
            cmode = configMode;
            sw1Timer = 0;
            beepN(skHz, 2);
            myDelay(500);
            break;
          case configMode:
            if (newWPM) {
              cmode = ditdahMode;
              // if we changed the WPM then return
              // to normal/dit-dah mode
              newWPM = 0;
              sw1Timer = 0;
              beepN(skHz, 1);
            } else {
              // otherwise continue to the next mode
              cmode = recallMode;
              sw1Timer = 0;
              beepN(skHz, 3);
            }
            myDelay(500);
            break;
          case recallMode:
            cmode = ditdahMode;
            sw1Timer = 0;
            beepN(skHz, 1);
            myDelay(500);
            break;
          default:
            doError();
        }
      }
    }
  }
  // check if the key mode has changed
  if (newKmode) {
    newKmode = 0;
    newCmode = 0;
    switch (kmode) {
      case iambicA:
        Serial.println("kmode is iambicA");
        break;
      case iambicB:
        Serial.println("kmode is iambicB");
        break;
      case straight:
        Serial.println("kmode is straight");
        break;
      default:
        doError();
    }
  }
  // check if the config mode has changed
  if (newCmode) {
    newCmode = 0;
    switch (cmode) {
      case ditdahMode:
        Serial.println("cmode is dit/dah");
        break;
      case configMode:
        Serial.println("cmode is config");
        // echo the current WPM
        beepWPM();
        break;
      case recallMode:
        Serial.println("cmode is recall");
        break;
      default:
        doError();
    }
  }
}

// recall and play a preprogrammed message
void recallMsg() {
  int x;
  // delay for simultaneous paddle check
  myDelay(100);
  x = ditPaddle + (dahPaddle<<1);
  switch (x) {
    case 1:
      // press dit paddle for msg1
      sendMorseMsg(msg1);
      break;
    case 2:
      // press dah paddle for msg2
      sendMorseMsg(msg2);
      break;
    case 3:
      // press both paddles for msg3
      sendMorseMsg(msg3);
      break;
    default:
      // no paddle is pressed
      break;
  }
}

// dit and dah paddle actions
void doDitDah() {
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
  delayMicroseconds(150);
}

// straight key
void doStright() {
  while (ditPaddle || dahPaddle) {
    digitalWrite(pinLed, HIGH);
    tone(pinBuzz, ddHz);
    readPaddles();
  }
  digitalWrite(pinLed, LOW);
  noTone(pinBuzz);
}

// play dit
void doDit() {
  digitalWrite(pinLed, HIGH);
  printc(".");
  wasDit = 1;
  tone(pinBuzz, ddHz);
  myDelay(ditLength);
  noTone(pinBuzz);
  digitalWrite(pinLed, LOW);
  myDelay(ditLength);
}

// play dah
void doDah() {
  digitalWrite(pinLed, HIGH);
  printc("-");
  wasDit = 0;
  tone(pinBuzz, ddHz);
  myDelay(ditLength*3);
  noTone(pinBuzz);
  digitalWrite(pinLed, LOW);
  myDelay(ditLength);
}

// delay by letter gap
void letterGap() {
  printc(" ");
  myDelay(ditLength*2);
}

// delay by word gap
void wordGap() {
  printc("/ ");
  myDelay(ditLength*4);
}

// delay with usb keep-alive
void myDelay(int dly) {
  unsigned long startTime = millis();
  unsigned long curTime = startTime;
  unsigned long endTime = startTime + dly;
  while(curTime < endTime){
    readPaddles();
    curTime = millis();
  }
}

// play beep code for current WPM
void beepWPM() {
  int N;
  // first digit
  if (actualWpm >= 20) {
    sendMorseChar('2');
    N = actualWpm - 20;
  } else {
    sendMorseChar('1');
    N = actualWpm - 10;
  }
  // second digit
  letterGap();
  N += 48;
  sendMorseChar((char)N);
}

// play N system beeps
void beepN(int freq, int N) {
  int i;
  for (i=0; i<N; i++) {
    beep(freq);
  }
}

// play system beep
void beep(int freq) {
  tone(pinBuzz, freq);
  myDelay(beepLen);
  noTone(pinBuzz);
  myDelay(beepDly);
}

// debug print
void printWPM() {
  if (debugOn) {
    Serial.print("Setting WPM to ");
    Serial.println(actualWpm);
  }
}

// debug print
void printc(char *str) {
  if (debugOn) {
    Serial.print(str);
  }
}

