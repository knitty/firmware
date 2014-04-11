# Knitty

## Interface

Currently we use a simple connection between our arduino and our superba 624 knitting machine. We only use SEL, IFDR, CSENSE and CREF (and of course GND).
The IFDR, CSENSE and CREF use an 10 KOhms pull-up resistor.

* CSENSE is connected to Interupt 0 (PIN 2)
* IFDR is connected to Interrupt 1 (PIN 3)
* CREF is connected to PIN 4
* SEL is connected to PIN 5

An Arduino shield is currently under development. If it's ready it will appear inside the hardware/ directory. It's also possible to use the Vin from the Arduino, if you connect the power supply to your arduino.

![Hardware Example](https://raw.githubusercontent.com/knitty/hardware-interface/master/hardware/example.png "Example Hardware")

## Software

You can easily open the Knitty.ino file with the arduino software and flash it onto your arduino. Be aware that your arduino should use 5V instead of 3,3 Volts.

## Attribution

This is the work of [ptflea](http://github.com/ptflea), [schinken](http://github.com/schinken) and [krisha](http://github.com/krisha).
