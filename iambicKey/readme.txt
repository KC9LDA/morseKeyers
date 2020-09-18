
A Morse Code Iambic Keyer

======

Pins:

Dit input is PIN D2

Dah input is PIN D3

LED/relay output is PIN D5

Switch sw1 input is PIN D7

Audio output is PIN D8

======

Libraries used:

Arduino.h
Wire.h

======

Modes:

There are 3 modes
Pressing and holding the sw1 pushbutton cycles through the 3 modes.
A beep pattern of 1, 2, or 3 beeps identifies the current mode.

1) normal: send morse via iambic keys
2) config: set the WPM speed
3) recall: recall and send a pre-programmed message

======

When in config mode:
Each press of the dit key increases the WPM
Each press of the dah key decreases the WPM
Pressing both dit and dah keys resets the WPM to its default value

Changing the WPM value is announced by a beep pattern.

WPM beep pattern examples:
10 = 1 low-tone beep
12 = 1 low-tone beeps followed by 2 higher-tone beeps
25 = 2 low-tone beeps followed by 5 higher-tone beeps

The WPM range is from 10WPM to 25WPM.
The initial default WPM setting is 15WPM.

======

When in recall mode:
pressing the dit paddle sends preprogrammed message #1
pressing the dah paddle sends preprogrammed message #2
pressing both paddles   sends preprogrammed message #3

The preprogrammed messages are part of the Arduino C source code.
You must edit and recompile iambicKey.ino in order to change them.

