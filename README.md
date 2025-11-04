# GreenhouseManagmentV3

There is a need for an expansive greenhouse plant care system that would address issues of temperature control and light management. Many systems are not connected together and do not provide visual indications of temperature or light. This system records the current values of these variables, while also writing into memory the lowest and highest values. A servo has been added to perform a sweeping motion whenever the temperature becomes too high.

Design

The Arduino Uno R3 microcontroller is connected to multiple sensors, an LED light and a servo:
DS1820 TO92 digital temperature sensor
Photoresistor
LED light
LCD display
Button
4xAA batteries
TIP120 transistor
Servo motor
There is an LCD display that shows the current value of the measurements. The values can be cycled through by pressing the button. The button cycles between the current temperature, current light level, highest and lowest temperature and then highest and lowest light.

Pin definitions:
 Temperature Sensor (DS18B20) -  Digital 13 
 Light Sensor - Analog A0 
 LED Output  - Digital 8 
 Button Input - Digital 2
TIP Output - Digital 9
Servo Output - Digital 10
 LCD RS - Digital 12 
 LCD Enable - Digital 11 
 LCD D4 - Digital 5 
 LCD D5 - Digital 4 
 LCD D6 - Digital 3 
 LCD D7 - Digital 6 


EEPROM layout

Address
Size
Name
Explanation
0
1 byte
Magic_Value
Indicates valid EEPROM data
1-4
4 bytes
highestTemp
Highest recorded temperature
5-8
4 bytes
highestLight
Highest recorded light level
9-12
4 bytes
lowestTemp
Lowest recorded temperature
13-16
4 bytes
lowestLight
Lowest recorded light level
17-20
4 bytes
counter
Last display mode


Interrupt instructions:

buttonISR():
- Debounces button input (500ms threshold)
- Sets `buttonPressed` flag for main loop processing
- Uses `millis()` for software debouncing

2. Function: `ISR(TIMER2_COMPA_vect)`
- Increments counter variables
- Sets `sensorReadFlag` every 500ms
- Sets `displayUpdateFlag` every 500ms
- Sets ‘servoUpdateFlag’ every 15ms

What works
	The system successfully measures the values from the sensors and is able to write them to memory to survive a power reset. The LCD screen switches values by button press. The servo activates when the temperature reaches a certain value.

	
Future improvements

Assuming that the light sensor would be in the same room as the plants, there needs to be a system that would make sure that the LED lights would not trigger the light sensor and create an ON/OFF loop.
A DC motor might be better for this task rather than a servo motor.
