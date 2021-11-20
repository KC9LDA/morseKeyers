
A Morse Code Iambic Keyer/Decoder

======

Pins:

Dit input is PIN D2

Dah input is PIN D3

Switch input is PIN D7

Audio output is PIN D8

======

Libraries used:

Arduino.h
Wire.h
OledI2C.h

======

Operation:

A single click of the pushbutton will enter the speed control (words-per-minute) menu. While in this menu the dit paddle decreases the speed and the dit paddle increases the speed. A second click of the menu pushbutton will enter the keyer mode selection menu. The choices are iambic-A and iambic-B. Since I normally use iambic-A, iambic-B is less tested. Let me know if you find any bugs in it.

When not in one of the menu modes, the trainer will translate from Morse code and print the resulting ascii character translations.

