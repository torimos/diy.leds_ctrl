#include <Arduino.h>
#include <SoftwareSerial.h>
#include "OneButton.h"
#include <EEPROM.h>

// Pin definitions
#define BUTTON_PIN PB0
#define ADC_PIN PB3
#define LED_PIN PB1
#define TX_PIN PB4

// Modes
enum Mode { MODE_STATIC, MODE_RANDOM_FADE, MODE_FIRE_FLICKER };
enum BrightnessMode { BR_MODE_INC, BR_MODE_DEC };
// Globals
const int brAddress = 0;
const int modeAddress = 1;

Mode mode;          // Current mode
BrightnessMode brMode; // Current brigthness regulaton mode (inc or dec)
short brightness;            // LED brightness (0-255)

SoftwareSerial Monitor(-1, TX_PIN, false); // Initialize SoftwareSerial for TX only
OneButton button(BUTTON_PIN, true);

// Function prototypes
void handleMode();
void staticMode();
void scheduledMode();
void randomFadeMode();
void fireFlickerMode();
void neonFlickerEffect();
void printMode();
void printBrightness();
void adjustBrightness();
void blinkLED(int times, int delayTime);

void saveSettings() {
  EEPROM.write(brAddress, (uint8_t)brightness);
  EEPROM.write(modeAddress, (uint8_t)mode);
}

void loadSettings() {
  mode = static_cast<Mode>(EEPROM.read(modeAddress));
  brightness = EEPROM.read(brAddress);
}

void LongPressStart(void *oneButton)
{
  brMode = static_cast<BrightnessMode>(!brMode);
}

// this function will be called when the button is released.
void LongPressStop(void *oneButton)
{
  adjustBrightness();
  saveSettings();
}

// this function will be called when the button is held down.
void DuringLongPress(void *oneButton)
{
  adjustBrightness();
}

void DblClick(void* OneButton)
{
  mode =  static_cast<Mode>(((uint8_t)mode + 1) % 3); // Cycle through modes
  printMode();
  saveSettings();
  blinkLED(1+((int)mode), 250);
  delay(500);
}

void blinkLED(int times, int delayTime) {
  for (int i = 0; i < times; i++) {
    analogWrite(LED_PIN, 16);
    delay(delayTime);
    analogWrite(LED_PIN, 0);
    delay(delayTime);
  }
}

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP); // Button with pull-up resistor
  pinMode(LED_PIN, OUTPUT);

  button.attachLongPressStart(LongPressStart, &button);
  button.attachDuringLongPress(DuringLongPress, &button);
  button.attachLongPressStop(LongPressStop, &button);
  button.attachDoubleClick(DblClick, &button);

  button.setLongPressIntervalMs(100);

  mode = MODE_STATIC;
  brightness = 255;

  Monitor.begin(9600);
  Monitor.println("System Initialized");
  
  loadSettings();

  if (brightness > 128)
  {
    brMode = BR_MODE_DEC;
  }
  else 
  {
    brMode = BR_MODE_INC;
  }
  printMode();
  printBrightness();

  blinkLED(1+((int)mode), 250);
  delay(1000);
}

void loop() {
  button.tick();
  handleMode();
}

void handleMode() {
  switch (mode) {
    case MODE_STATIC:
      staticMode();
      break;
    case MODE_RANDOM_FADE:
      randomFadeMode();
      break;
    case MODE_FIRE_FLICKER:
      fireFlickerMode();
      break;
  }
}

void staticMode() {
    analogWrite(LED_PIN, brightness); // Turn LED ON at the defined brightness
}

void randomFadeMode() {
  static int fadeValue = 0;
  static int fadeDirection = 1; // 1 for increasing, -1 for decreasing
  static uint32_t fadeDelay = 20;
  static unsigned long lastUpdate = 0; // Last update timestamp

  if ((millis() - lastUpdate) > fadeDelay) { // Non-blocking delay for fade
    lastUpdate = millis();
    fadeValue += fadeDirection;
    if (fadeValue <= 0 || fadeValue >= 255) {
      fadeDirection = -fadeDirection; // Reverse direction
      fadeDelay = random(5,20);
      if (fadeValue <=0)
        fadeValue = 0;
      if (fadeValue > 255)
        fadeValue = 255;
    }
    int scaledValue = map(fadeValue, 0, 255, 0, brightness); // Scale by brightness
    analogWrite(LED_PIN, scaledValue);
  }
}

void fireFlickerMode() {
  static int currentBrightness = 25; // Start at a base brightness
  static int targetBrightness = random(25, 255); // Initial random target
  static unsigned long lastUpdateTime = 0; // Time of the last update
  static int step = 0; // Step value for smooth transitions
  static int stepsRemaining = 0; // Number of steps left in the transition

  unsigned long currentTime = millis();

  // Time interval between brightness updates
  const unsigned long updateInterval = 50;

  if (currentTime - lastUpdateTime > updateInterval) {
    lastUpdateTime = currentTime;

    if (stepsRemaining > 0) {
      // Gradually move towards the target brightness
      currentBrightness += step;
      stepsRemaining--;

      // Constrain brightness to a minimum value
      int adjustedBrightness = currentBrightness < 32 ? 32 : currentBrightness;
      
      int scaledValue = map(adjustedBrightness, 0, 255, 0, brightness); // Scale by brightness
      analogWrite(LED_PIN, scaledValue);
    } else {
      // When the transition is complete, set a new target
      targetBrightness = random(25, 255);
      stepsRemaining = 10; // Number of steps for the transition
      step = (targetBrightness - currentBrightness) / stepsRemaining;
    }
  }
}

void adjustBrightness() {
  static unsigned long lastUpdate = 0; // Last update timestamp
  const int brStep = 2;
  const int brUpdateInterval = 30;

  int adcValue = analogRead(ADC_PIN);
  int maxBrightness = map(adcValue, 0, 1023, 0, 255);
  if ((millis()-lastUpdate)>brUpdateInterval)
  {
    lastUpdate = millis();
    if (brMode == BR_MODE_INC) //inc
    {
      brightness += brStep; // Increment brightness
      if (brightness >= maxBrightness) brightness = maxBrightness; // Wrap around
    }
    else if (brMode == BR_MODE_DEC) //dec
    {
      brightness -= brStep; // Increment brightness
      if (brightness <= 0) brightness = 0; // Wrap around
    }

    printBrightness();
  }
}

void printBrightness() {
  Monitor.print("Brightness: ");
  Monitor.println(brightness);
}

void printMode() {
  // Output current mode to SoftwareSerial
  Monitor.print("Current Mode: ");
  switch (mode) {
    case MODE_STATIC:
      Monitor.println("Static");
      break;
    case MODE_RANDOM_FADE:
      Monitor.println("Random Fade In/Out");
      break;
    case MODE_FIRE_FLICKER:
      Monitor.println("Fire Flicker");
      break;
    default:
      Monitor.println("Unknown");
      break;
  }
}