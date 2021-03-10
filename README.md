# Arduino Midi Ribbon Controller

[![Demo](/3.jpg)](https://www.youtube.com/watch?v=UU3WkZmNJEc)

This project was inspired by Wintergatan's Modulin https://www.youtube.com/watch?v=MUdWeBYe3GY

## Features

* 2 x 24 quantized notes on a 50cm long neck
* Able to switch between all 7 music modes while retaining quantization
* +/- Semitone Tranpose and +/- Octave Tranpose
* Finger pressure controls modulation
* Volume controllable by adjusting the pot at the end of the neck
* LED reacts to finger position
* Pitchbend by using the analog stick

[Demo](https://youtu.be/UU3WkZmNJEc)

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
![Schematics](/schematics.png)

Note : In the prototyping board, all nodes in a black rectangle are connected together

Note : The softpots are placed over the force sensitive resistors ( see picture below)


### Pins connection

* Softpots : A4,A2
* FSRs : A5,A3
* Pot : A8
* Analog Stick : A7
* Neopixel : D21

### First time calibration
* Set the A0 potentiometer all the way down on the midi shield.
* Uncomment Serial.begin(115200) and comment Serial.begin(31250);
* Uncomment calibrate() at line 168
* To calibrate the softpots, open a serial console. When it says "waiting" press on one of the softstick and set the A0 potentiometer up on the midi shield. Once you see the confirmation in the console output turn it back down and proceed back to the next fret. Repeat for N_FRET times and repeat the same for the second softpot. When it's done the program will write the calibration in to the EEPROM and you may comment the line 168 and switch the serial to 31250 baud again. 
* The notes that are close to the Arduino are lower

## Manual
* Semitone Transpose : Pressing one of black buttons on the midi shield will transpose up and the other black button will transpose down
* Octave Transpose : Pressing the middle button AND one of the black button with do an octave transpose
* Change musical mode : Turning the "lower" knob will change the music mode. When switching modes, the LEDs will light up according the mode you've chosen (1-7)
* You need to twist the "upper" knob all the way up for it to work. This is a bug
* Change volume : use the volume knob at the end of the stick
* Pitch bend : Use the analog stick for pitchben
* Modulation : Modulation is controlled by how hard you press on the stick, the harder you press the more modulation.
* The midi out of the midi shield need to be plugged into your audio interface/midi converter
* Power the Arduino either by pluggin the USB cable or through battery




![Complete](/1.jpg)
![Closeup](/2.jpg)
