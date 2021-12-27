

A Vband paddle-to-USB interface using an AdaFruit Trinket M0

======

Description:

Connect a Morse code paddle to a computer USB input
Pressing the Dit paddle causes right control key code to be sent over USB
Pressing the Dah paddle causes left  control key code to be sent over USB

Software on the Vband server will interpret the key codes back to dits an dahs
Connect to the Vband server @ https://hamradio.solutions/vband/

======

AdaFruit Trinket Pins:

3   -- Dit input
0   -- Dah input
GND -- ground

======

Libraries used:

#include <Adafruit_DotStar.h>   // only needed to turn off LED
#include <Keyboard.h>           // emulate USB keyboard

======

Additional Info:


