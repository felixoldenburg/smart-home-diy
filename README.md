# Smart Home DIY

Tracking the journey of creating custom build, µController based Smart Home components.
Wiring them up to automatise repetitive scenarios for light and temperature.

## Motivation
Just picking up an old box with electronic components got me started refreshing knowledge about transistors, µControllers, etc.

As many others I like to learn on the fly while working towards something new to learn or build. Instead of only digging into books and articles without the practical part.

It all started with ...
"Hey, I got this cheap set of simple, but digital heater thermostats. I wonder if these could be modded to be controlled remotely."

## Steps
### Analysis and preparation of heater thermostat
1. Analyse heater thermostat -> It's using a rotary encoder, with 3 pins
2. Disassembling and re-assembling didn't damage the device! Phew..
3. Desolder rotary encoder and solder wires to the board AND DON'T damage these incredibly tiny SMD parts

### How to simulate rotary encoder signals with an Arduino?
4. Read into topic of rotary encoder signals
5. Get inspirations, read about related material and instructions
6. Found code snippet that can simulate rotary encoder impulses via PWM

### Hook up the thermostat and set temperature
7. Wire up the thermostat and code, build, upload and test setting of temperature
8. Add buttons for manual single impulses, as the rotary encoder is also used for navigating the menu of the ts
9. Learn about pull-up and pull-down resistors necessary for proper button & LED wiring

The are microcontroller experimentation boards nowadays like the Arduino platform coming with all kinds of comforts.
No need to take care of power supply and regulations, no need for wiring up a serial programmer, etc.
Well, I really only know little and have few experience with electronic components, BUT
using the convenience of a platform like Arduino and quickly be able to proto type something really makes all the difference.
Quicker achievements, more motivation!

### Setup of a Adafruit HUZZAH ESP8266 
As I wanted to keep my Arduino Uno for experimentation and needed some small device, ideally with some remote functionality, I decided to give the Adafruit HUZZAH ESP8266 a try.
The Huzzah is well supported by the Arduino IDE and comes with a Wifi chip on board. Looking back chosing a Wifi based chip wasn't the best choice for a battery based setup. Wifi has a significantly higher energy consumption than 433Mhz, Bluetooth or LoRa based solutions.
Still I'll continue to use it for now and will work with usb powered setup and try some deep sleep setup where the Huzzah wakes up every n minutes to send/receive data and control commands from a Raspberry Pi.
See the link section on how to setup the Huzzah with the Arduino IDE.
To start working with json based http calls the [ArduinoJson library](https://arduinojson.org/) is necessary which can be installed with the library manager of Arduino IDE

### Scheduled wake up
In order to safe energy on letting the Huzzah idle forever the chip knows different kinds of sleep modes.
For the temperature control it's sufficient to only wake up every n minutes. In my case the controller should send the current temperature data to a time series db and read the target temperature it's supposed to set.
Head over to https://randomnerdtutorials.com/esp8266-deep-sleep-with-arduino-ide/ for some nice tutorial and overview about the different sleep modes and how to use them.
In my case I have a `ESP.deepSleep(10e6);` (the interval param is in µ seconds) at the end of the setup() method to set a sleep timer for 10s. 10s is for testing the code. Later a couple of minutes should suffice. This depends on how fine granular the desired tmp data should be recorded and how often the tmp should be adjusted.
Normally for the controller to be restarted we'd need to pull the RST pin to ground (RST is always HIGH).
The timer will send a LOW signal to GPIO16. So if this pin is wired with the RST pin, the timer reset the controller after the interval and the controller will execute the setup() method again.

## Concept
* Controlling lights and temperature in flat
* Low cost and low energy
* Track and optimise heating to save money
* Scenario - A Scenario defines a predefined set of actions or light setup

## Characteristics
- Don't talk to the internet! Specifically to any butt provider or company
- Aiming for simplicity

## Misc & Considerations
- Leverage iOS Homekit as UI

## Links
[Setup Arduino IDE for Adafruit Feather Huzzah ESP8266](https://learn.adafruit.com/adafruit-feather-huzzah-esp8266/using-arduino-ide)
[Getting started with the ESP8266 and DHT22 sensor](https://www.losant.com/blog/getting-started-with-the-esp8266-and-dht22-sensor)
[Install Virtual COM Port driver for Arduino IDE for OSX](https://www.ftdichip.com/Drivers/VCP.htm)

## Paths
/Applications/Arduino.app/Contents/Java/hardware/tools/avr
/Users/f.oldenburg/Library/Arduino15
/Users/f.oldenburg/Documents/Arduino
