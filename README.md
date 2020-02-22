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

### How to wire 

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
