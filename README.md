# Arduino Midi Ribbon Controller

[![Demo](/3.jpg)](https://www.youtube.com/watch?v=UU3WkZmNJEc)

This project was inspired by Wintergatan's Modulin https://www.youtube.com/watch?v=MUdWeBYe3GY

## Main components

 
* [2 x Softpot Membrane](https://www.sparkfun.com/products/8681)
* [2 x Force Sensitive Resistor](https://www.sparkfun.com/products/9674)
* 1 x Arduino Mega
* [1 x Sparkfun Midi Shield](https://www.sparkfun.com/products/12898)
* 1 x 10K pot (volume control)
* 1 x 1M 60LED Neopixel
* 1 x Analog Joystick (pitchbend)

Theory by : https://synthhacker.blogspot.ca/2016/03/diy-midi-ribbon-controller.html
Code adapted from https://github.com/deanm1278/arduinoRibbonController/blob/master/synth_v3.ino

## Schematics 
![Schematics](/schematics_bb.png)

Note : The softpots are placed over the force sensitive resistors ( see picture below)

### Pins connection

* Softpots : A4,A2
* FSRs : A5,A3
* Pot : A8
* Analog Stick : A7
* Neopixel : D21

### First time calibration
* Uncomment Serial.begin(115200) and comment Serial.begin(31250);
* Uncomment calibrate() at line 168
* To calibrate the softpots, open a serial console. When it says "waiting" press on one of the softstick and press the middle button on the midi shield. Repeat for N_FRET times and repeat the same for the second softpot. When it's done the program will write the calibration in to the EEPROM and you may comment the line 168 and switch the serial to 31250 baud again. 

## Features

* 2 x 24 quantized notes on a 50cm long neck
* Able to switch between all 7 music modes while retaining quantization
* +/- Semitone Tranpose and +/- Octave Tranpose
* Finger pressure controls modulation
* Volume controllable by adjusting the pot at the end of the neck
* LED reacts to finger position
* Pitchbend by using the analog stick

[Demo](https://youtu.be/UU3WkZmNJEc)



![Complete](/1.jpg)
![Closeup](/2.jpg)
