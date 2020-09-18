
// ============================================================
// A Morse code keyer with USB keyboard and LCD
//
// (c) Scott Baker KJ7NLA
// ============================================================

#include <hidboot.h>
#include <SPI.h>
#include <LiquidCrystal_I2C.h>

#define BAUDRATE 115200
#define BUFDEPTH 128

const int pinLed  = 5;      // LED pin (to relay)
const int pinBuzz = 8;      // buzzer/speaker pin

const int debugOn = 1;      // print debug messages

const int minWpm  = 10;     // min speed
const int midWpm  = 15;     // initial speed
const int maxWpm  = 25;     // max speed

const int skHz    = 220;    // beep @A4 swap keys
const int dkHz    = 262;    // beep @C4 default keys
const int dnHz    = 440;    // beep @A5 WPM-
const int upHz    = 523;    // beep @C5 WPM+
const int ddHz    = 659;    // beep @E5 dit/dah

const int beepLen = 69;     // system beep  length
const int beepDly = 99;     // system delay length

int col           = 0;      // LCD column
int row           = 0;      // LCD row
int headPtr       = 0;      // ascii buffer head pointer
int savePtr       = 0;      // saved buffer head pointer
int tailPtr       = 0;      // ascii buffer tail pointer
int wasDit        = 0;      // prev state was dit
int actualWpm;              // words per minute
int ditLength;              // dit length in ms

int configMode    = 0;      // configuration mode
int readyToSend   = 0;      // ready to send morse
int recallLine    = 0;      // recall previous line
uint8_t wpmKey    = 0;      // update WPM speed
uint8_t abuf[BUFDEPTH];     // ascii buffer
char mcode[8];              // morse code buffer

LiquidCrystal_I2C lcd(0x27,20,4);

class KbdRptParser : public KeyboardReportParser {
  void OnKeyDown (uint8_t mod, uint8_t key);
};

void initWpm(void);
void getDitLength(void);
void getPrevline(void);
void sendMorseLine(void);
void sendMorseChar(char acode);
void morseLookup(char acode);
void updateWpm(uint8_t key);
void doDit(void);
void doDah(void);
void getRowCol(void);
void clearAll(void);
void doError(void);
int8_t uppercase(int8_t cx);

void(*resetFunc) (void) = 0;  // reset function at address 0

void KbdRptParser::OnKeyDown(uint8_t mod, uint8_t key) {
  uint8_t cx;
  if (!configMode) {
    // check for escape
    if (key == 41) {
      // enter configuration mode
      lcd.setCursor(0,0);
      lcd.print("Config mode         ");
      lcd.setCursor(0,1);
      lcd.print("up-arrow for WPM++  ");
      lcd.setCursor(0,2);
      lcd.print("dn-arrow for WPM--  ");
      lcd.setCursor(0,3);
      lcd.print("Current WPM = ");
      lcd.print(actualWpm);
      lcd.print(" ");
      configMode = 1;
      return;
    // check for backspace
    } else if (key == 42) {
      // remove the character from the buffer
      headPtr--;
      // do not allow less than zero
      if (headPtr < 0) {headPtr = 0;}
      getRowCol();
      // erase previous character
      lcd.setCursor(col,row);
      lcd.print(' ');
      return;
    // check for line recall (up-arrow)
    } else if (key == 82) {
      recallLine = 1;
      return;
    // check for clear (dn-arrow)
    } else if (key == 81) {
      // clear the LCD screen and reset pointers
      clearAll();
      return;
    } else {
      cx = OemToAscii(mod, key);
      if (cx) {
        // handle ascii character cases
        if (cx == 19) {
          // carriage return
          readyToSend = 1;    // ready to send morse
          return;
        } else {
          // convert to upper case
          cx = uppercase(cx);
          // print the character
          lcd.setCursor(col,row);
          lcd.print((char)cx);
          // push it to the character buffer
          abuf[headPtr++] = cx;
          getRowCol();
          // check for buffer overflow error
          if (headPtr > 80) {doError();}
        }
      }
    }
  } else {
    // check for escape
    if (key == 41) {
      // exit configuration mode
      lcd.clear();
      configMode = 0;
      // if there were characters in the buffer
      // then recall the line after return
      if (headPtr) {
        savePtr = headPtr;
        recallLine = 1;
      }
      return;
    } else if ((key == 81) || (key == 82)) {
      // update WPM speed
      wpmKey = key;
    }
  }
}

USB Usb;
HIDBoot<USB_HID_PROTOCOL_KEYBOARD> HidKeyboard(&Usb);
KbdRptParser ParseKbd;

// Arduino init
//
void setup() {
  int gotErr = 0;
  pinMode(pinLed,  OUTPUT);
  pinMode(pinBuzz, OUTPUT);
  configMode = 0;            // normal mode
  readyToSend = 0;           // not ready to send
  recallLine  = 0;           // do not recall line
  lcd.init();                // init LCD
  lcd.backlight();
  Serial.begin(BAUDRATE);    // init serial console
  lcd.print("USB init ");
  if (Usb.Init() == -1) {
    lcd.print("** ERROR **");
    gotErr = 1;
  }
  delay(200);
  HidKeyboard.SetReportParser(0, &ParseKbd);
  // clear LCD if no error
  if (!gotErr) {lcd.clear();}
  initWpm();
}

// Arduino main loop
//
void loop() {
  if (wpmKey) {
    updateWpm(wpmKey);
  } else if (readyToSend) {
    sendMorseLine();
  } else if (recallLine) {
    getPrevline();
  } else {
    Usb.Task();
    delay(1);
  }
}

void getRowCol() {
  row = 0;
  if (headPtr > 19) {row = 1;}
  if (headPtr > 39) {row = 2;}
  if (headPtr > 59) {row = 3;}
  if (headPtr > 79) {row = 0;}
  col = headPtr;
  while (col > 19) {col -= 20;}
}

// clear the LCD screen and reset pointers
void clearAll() {
  headPtr = 0;
  tailPtr = 0;
  getRowCol();
  lcd.clear();
}

// handle error
//
void doError() {
  lcd.setCursor(0,0);
  lcd.print("ERROR:              ");
  lcd.setCursor(0,1);
  lcd.print("Buffer Overflow     ");
  delay(5000);
  resetFunc();   // reset the Arduino
}

// display the previous line
//
void getPrevline() {
  char ch;
  uint8_t count;
  recallLine = 0;
  if (savePtr == 0) {return;}
  tailPtr = 0;
  headPtr = 0;
  count = savePtr;
  while (count) {
    count--;
    // get each ascii character
    ch = (char)abuf[headPtr];
    getRowCol();
    lcd.setCursor(col,row);
    lcd.print(ch);
    headPtr++;
  }
  getRowCol();
}

// send a line of Morse code
//
void sendMorseLine() {
  char acode;
  readyToSend = 0;
  if (!headPtr) {
    // nothing to send so
    // clear the LCD screen and reset pointers
    clearAll();
    return;
  }
  savePtr = headPtr;
  // check if there is a character to send
  while (headPtr) {
    headPtr--;
    // pop from the character buffer
    acode = (char)abuf[tailPtr++];
    // convert the character to Morse code
    sendMorseChar(acode);
  }
  // clear the LCD screen and reset pointers
  clearAll();
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
void updateWpm(uint8_t key) {
  wpmKey = 0;
  if (key == 82) {
    // increase WPM speed (up-arrow)
    actualWpm++;
    if (actualWpm > maxWpm) {
      actualWpm = maxWpm;
      lcd.setCursor(0,3);
      lcd.print("At max WPM = 25     ");
      return;
    }
    beep(upHz);
  } else {
    // decrease WPM speed (down-arrow)
    actualWpm--;
    if (actualWpm < minWpm) {
      actualWpm = minWpm;
      lcd.setCursor(0,3);
      lcd.print("At min WPM = 10     ");
      return;
    }
    beep(dnHz);
  }
  getDitLength();
  printWPM();
}

// Calculate dit length in ms
void getDitLength() {
  ditLength = 1200 / actualWpm;
}

// play dit
void doDit() {
  digitalWrite(pinLed, HIGH);
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
  wasDit = 0;
  tone(pinBuzz, ddHz);
  myDelay(ditLength*3);
  noTone(pinBuzz);
  digitalWrite(pinLed, LOW);
  myDelay(ditLength);
}

// delay by letter gap
void letterGap() {
  myDelay(ditLength*2);
}

// delay by word gap
void wordGap() {
  myDelay(ditLength*4);
}

// delay with usb keep-alive
void myDelay(int dly) {
  unsigned long startTime = millis();
  unsigned long curTime = startTime;
  unsigned long endTime = startTime + dly;
  while(curTime < endTime){
    Usb.Task();
    delay(1);
    curTime = millis();
  }
}

// play system beep
void beep(int freq) {
  tone(pinBuzz, freq);
  myDelay(beepLen);
  noTone(pinBuzz);
  myDelay(beepDly);
}

// convert to upper case
int8_t uppercase(int8_t cx) {
  if ((cx>96) && (cx<123)) {
    cx -= 32;
  }
  return(cx);
}

// debug print
void printWPM() {
  if (debugOn) {
    lcd.setCursor(0,3);
    lcd.print("Setting WPM to ");
    lcd.print(actualWpm);
    lcd.print(" ");
  }
}

// debug print
void printc(char *str) {
  if (debugOn) {
    Serial.print(str);
  }
}

