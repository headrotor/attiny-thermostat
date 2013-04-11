
This is AVR-GCC code for a  [EMSL Art
Controller](http://www.evilmadscientist.com/2012/artcontroller/) modified to
thermostatically control the output relay. A TC74 temperature sensor
turns on the relay when the temperature rises above the set point. The
relay can be connected in Normally-On mode for heaters or Normally-Off
mode for coolers or fans.


Designed to be used with the Art Controller relay board from EMSL
http://www.evilmadscientist.com/2012/artcontroller/
Requires a TC74 temperature sensor connected as follows:


|Vdd | PB3|
| --- | --- | ---
|SCK | PB2|
|GND | PB1|
|SDA | PB0|


PD1 is the TX output of the UART used for debugging. Don't close the
X60 switch or the X10 switch!

This code has been adapted from the [EMSL Art Controller
code](http://wiki.evilmadscience.com/Art_Controller), and includes a
bit-bang I2C code based on this [blog
post](http://codinglab.blogspot.com/2008/10/i2c-on-avr-using-bit-banging.html)
by ["Raul"](http://www.blogger.com/profile/05112542436303049493)

More information on the hardware can be found at [this blog post on rotormind.com](http://rotormind.com/blog/2013/Art-Controller-Thermostat)




the main file is in arttherm.c
