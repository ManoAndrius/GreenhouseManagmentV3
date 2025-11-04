#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>
#include <LiquidCrystal.h>
#include <Servo.h>  

// Pin definitions
#define LIGHT_PIN A0
#define ONE_WIRE_BUS 13
const int ledPin = 8;
const int buttonPin = 2;
const int servoPin = 10;
const int TIPpin = 9;

// EEPROM addresses
#define MAGIC_ADDR      0
#define MAGIC_VALUE     0x42
#define HIGH_TEMP_ADDR  1
#define HIGH_LIGHT_ADDR 5
#define LOW_TEMP_ADDR   9
#define LOW_LIGHT_ADDR  13
#define BUTTON_ADDR     17

// Hardware objects
LiquidCrystal lcd(12, 11, 5, 4, 3, 6);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
Servo servo;

// Record tracking variables
float highestTemp = -100.0;
float lowestTemp = 1000.0;   
float highestLight = 0.0;
float lowestLight = 1000.0; 
int counter = 0;

// Current sensor readings
volatile float currentTemp = 0.0;
volatile float currentLight = 0.0;

// Display state tracking
float tempLast = -100.0;
float lightLast = -1.0;
int lastCounter = -1;

// Event flags
volatile bool buttonPressed = false;
volatile bool sensorReadFlag = false;
volatile bool displayUpdateFlag = false;
volatile bool servoUpdateFlag = false;

// Timer counters
volatile uint16_t sensorTimerCount = 0;
volatile uint16_t displayTimerCount = 0;
volatile uint16_t servoTimerCount = 0;
const uint16_t SENSOR_INTERVAL = 500;   
const uint16_t DISPLAY_INTERVAL = 500;   
const uint16_t SERVO_INTERVAL = 15;

// Servo variables
int servoPos = 0;
bool servoActive = false;
bool servoForward = true;

// Button debouncing
volatile unsigned long lastButtonTime = 0;
const unsigned long debounceDelay = 500;

void buttonISR() {
  unsigned long currentTime = millis();
  if (currentTime - lastButtonTime > debounceDelay) {
    buttonPressed = true;
    lastButtonTime = currentTime;
  }
}

void timer2_compa_handler() {
  sensorTimerCount++;
  displayTimerCount++;
  servoTimerCount++;
  
  if (sensorTimerCount >= SENSOR_INTERVAL) {
    sensorReadFlag = true;
    sensorTimerCount = 0;
  }
  
  if (displayTimerCount >= DISPLAY_INTERVAL) {
    displayUpdateFlag = true;
    displayTimerCount = 0;
  }
  
  if (servoTimerCount >= SERVO_INTERVAL) {
    servoUpdateFlag = true;
    servoTimerCount = 0;
  }
}

ISR(TIMER2_COMPA_vect) {
  timer2_compa_handler();
}


void setup() {
  sensors.begin();
  lcd.begin(16, 2);
  
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(TIPpin, OUTPUT);
  
  servo.attach(servoPin);
  servo.write(0);
  servoPos = 0;

  lcd.clear();
  lcd.print("Initializing...");
  lcd.setCursor(0, 1);
  lcd.print("Temp & Light");
  
  attachInterrupt(digitalPinToInterrupt(buttonPin), buttonISR, FALLING);
  setupTimer2();  

  loadEEPROMData();
  
  delay(1000);
  lcd.clear();
  
  readSensors();
  updateDisplay();
}

void loop() {
  if (buttonPressed) {
    handleButtonPress();
    buttonPressed = false;
  }
  
  if (sensorReadFlag) {
    readSensors();
    checkAndUpdateRecords();
    controlLED();
    controlServoLogic();
    sensorReadFlag = false;
  }
  
  if (displayUpdateFlag) {
    updateDisplay();
    displayUpdateFlag = false;
  }
  
  if (servoUpdateFlag) {
    updateServoPosition();
    servoUpdateFlag = false;
  }
}

void readSensors() {
  currentLight = analogRead(LIGHT_PIN);
  
  sensors.requestTemperatures();
  currentTemp = sensors.getTempCByIndex(0); 
}


void checkAndUpdateRecords() {
  if (currentTemp > highestTemp) {
    highestTemp = currentTemp;
    EEPROM.put(HIGH_TEMP_ADDR, highestTemp);
  }
  
  if (currentTemp < lowestTemp) {
    lowestTemp = currentTemp;
    EEPROM.put(LOW_TEMP_ADDR, lowestTemp);
  }
  
  if (currentLight > highestLight) {
    highestLight = currentLight;
    EEPROM.put(HIGH_LIGHT_ADDR, highestLight);
  }
  
  if (currentLight < lowestLight) {
    lowestLight = currentLight;
    EEPROM.put(LOW_LIGHT_ADDR, lowestLight);
  }
}

void controlLED() {
  if (currentLight < 700) {
    digitalWrite(ledPin, HIGH);
  } else {
    digitalWrite(ledPin, LOW);
  }
}

void controlServoLogic() {
  if (currentTemp > 24) {
    digitalWrite(TIPpin, HIGH);  
    if (!servoActive) {
      servoActive = true;
      servoForward = true;
      servoPos = 0;
    }
  } else {
    digitalWrite(TIPpin, LOW);
    if (servoActive) {
      servoActive = false;
    }
  }
}

void updateServoPosition() {
  if (!servoActive && servoPos == 0) {
    return;  // Servo is idle at position 0
  }
  
  if (servoActive) {
    if (servoForward) {
      servoPos++;
      if (servoPos >= 180) {
        servoForward = false;  // Reverse direction
      }
    } else {
      servoPos--;
      if (servoPos <= 0) {
        servoForward = true;   // Forward again
      }
    }
    servo.write(servoPos);
  }
}

void handleButtonPress() {
  counter++;
  if (counter > 5) { 
    counter = 0;
  }
  lastCounter = -1;
  displayUpdateFlag = true;
  EEPROM.put(BUTTON_ADDR, counter);
}

void updateDisplay() {
  bool needsUpdate = (lastCounter != counter);
  switch (counter) {
    case 0: 
      if (needsUpdate || tempLast != currentTemp) {
        tempLast = currentTemp;
        printData("TEMPERATURE:", " C", currentTemp, lcd);
        lastCounter = counter;
      }
      break;
      
    case 1: 
      if (needsUpdate || lightLast != currentLight) {
        lightLast = currentLight;
        printData("LIGHT LVL:", "", currentLight, lcd);
        lastCounter = counter;
      }
      break;
      
    case 2: 
      printData("HIGHEST TEMP:", " C", highestTemp, lcd);
      lastCounter = counter;
      break;
      
    case 3:
      printData("LOWEST TEMP:", " C", lowestTemp, lcd);
      lastCounter = counter;
      break;
      
    case 4: 
      printData("HIGHEST LIGHT:", "", highestLight, lcd);
      lastCounter = counter;
      break;
      
    case 5: 
      printData("LOWEST LIGHT:", "", lowestLight, lcd);
      lastCounter = counter;
      break;
  }
}

void printData(String text, String text2, float value, LiquidCrystal &lcd) {
  lcd.clear();
  lcd.print(text);
  lcd.setCursor(0, 1);
  lcd.print(value, 1);
  lcd.print(text2);
}

void loadEEPROMData() {
  if (EEPROM.read(MAGIC_ADDR) == MAGIC_VALUE) {
    EEPROM.get(HIGH_TEMP_ADDR, highestTemp);
    EEPROM.get(LOW_TEMP_ADDR, lowestTemp);
    EEPROM.get(HIGH_LIGHT_ADDR, highestLight);
    EEPROM.get(LOW_LIGHT_ADDR, lowestLight);
    EEPROM.get(BUTTON_ADDR, counter);
  } else {
    EEPROM.write(MAGIC_ADDR, MAGIC_VALUE);
    EEPROM.put(HIGH_TEMP_ADDR, highestTemp);
    EEPROM.put(LOW_TEMP_ADDR, lowestTemp);
    EEPROM.put(HIGH_LIGHT_ADDR, highestLight);
    EEPROM.put(LOW_LIGHT_ADDR, lowestLight);
    EEPROM.put(BUTTON_ADDR, counter);
  }
}

void setupTimer2() {
  noInterrupts();
  
  TCCR2A = 0;
  TCCR2B = 0;
  TCNT2 = 0;
  
  // Set compare match register for 1ms (1000 Hz)
  // Timer2 is 8-bit, so we use different prescaler
  // Formula: (16MHz / (prescaler * frequency)) - 1
  // (16000000 / (128 * 1000)) - 1 = 124
  OCR2A = 124;
  
  // Configure Timer2
  TCCR2A |= (1 << WGM21);   // CTC mode
  TCCR2B |= (1 << CS22) | (1 << CS20);  // 128 prescaler
  TIMSK2 |= (1 << OCIE2A);  // Enable compare interrupt
  
  interrupts();
}