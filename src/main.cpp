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
enum Mode { MODE_STATIC, MODE_SCHEDULED, MODE_RANDOM_FADE, MODE_NEON_FLICKER, MODE_FIRE_FLICKER };
enum BrightnessMode { BR_MODE_INC, BR_MODE_DEC };
// Globals
const int brAddress = 0;
const int modeAddress = 1;
const int brStep = 8;
const int brRefreshDelay = 30;

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
  mode =  static_cast<Mode>(((uint8_t)mode + 1) % 5); // Cycle through modes
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
    case MODE_SCHEDULED:
      scheduledMode();
      break;
    case MODE_RANDOM_FADE:
      randomFadeMode();
      break;
    case MODE_NEON_FLICKER:
      neonFlickerEffect();
      break;
    case MODE_FIRE_FLICKER:
      fireFlickerMode();
      break;
  }
}

void staticMode() {
    analogWrite(LED_PIN, brightness); // Turn LED ON at the defined brightness
}

void scheduledMode() {
  const unsigned long period = 60000; // Total period in milliseconds (5 minutes)

  // Duty cycle variables
  static unsigned long onTime = 0;           // LED ON time
  static unsigned long offTime = 0;          // LED OFF time

  // Timing variables
  static unsigned long previousMillis = 0;   // Tracks the last timing event
  static bool ledState = false;              // Tracks LED state (ON/OFF)

   // Read ADC value and map it to duty cycle percentage (10% to 90%)
  int adcValue = analogRead(ADC_PIN);
  int dutyCycle = map(adcValue, 0, 1023, 5, 100);

  // Calculate ON and OFF times based on the duty cycle
  onTime = (period * dutyCycle) / 100;    // ON time in milliseconds
  offTime = period - onTime;             // OFF time in milliseconds


  // Get the current time
  unsigned long currentMillis = millis();
  unsigned long targetTime = dutyCycle*period/100;
  Monitor.print(targetTime);
  Monitor.print(" ");
  if (!ledState)
  {
    Monitor.print(period-(currentMillis - previousMillis));
    Monitor.print(" next is on. ");
  }
  if (ledState)
  {
    Monitor.print(period-(currentMillis - previousMillis));
    Monitor.print(" next is off");
  }
  Monitor.println();

  // Manage LED state based on timing and duty cycle
  if (ledState && (currentMillis - previousMillis >= onTime)) 
  {
    // If LED is ON and the ON duration has elapsed, turn it OFF
    ledState = false;
    previousMillis = currentMillis;  // Reset timing
    analogWrite(LED_PIN, 0);         // Turn LED OFF
  } 
  else if (!ledState && (currentMillis - previousMillis >= offTime)) 
  {
    // If LED is OFF and the OFF duration has elapsed, turn it ON
    ledState = true;
    previousMillis = currentMillis;  // Reset timing
    analogWrite(LED_PIN, brightness); // Turn LED ON at the defined brightness
  }
}

void randomFadeMode() {
  static int fadeValue = 0;
  static int fadeDirection = 1; // 1 for increasing, -1 for decreasing
  static uint32_t fadeDelay = 10;
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
  static int currentBrightness = brightness / 10; // Start at a base brightness
  static int targetBrightness = random(brightness / 10, brightness); // Initial random target
  static unsigned long lastUpdateTime = 0; // Time of the last update
  static int step = 0; // Step value for smooth transitions
  static int stepsRemaining = 0; // Number of steps left in the transition

  unsigned long currentTime = millis();

  // Time interval between brightness updates
  const unsigned long updateInterval = 30;

  if (currentTime - lastUpdateTime > updateInterval) {
    lastUpdateTime = currentTime;

    if (stepsRemaining > 0) {
      // Gradually move towards the target brightness
      currentBrightness += step;
      stepsRemaining--;

      // Constrain brightness to a minimum value
      int adjustedBrightness = currentBrightness < 32 ? 32 : currentBrightness;
      analogWrite(LED_PIN, adjustedBrightness);
    } else {
      // When the transition is complete, set a new target
      targetBrightness = random(brightness / 10, brightness);
      stepsRemaining = 10; // Number of steps for the transition
      step = (targetBrightness - currentBrightness) / stepsRemaining;
    }
  }
}

void neonFlickerEffect2() {
  static unsigned long lastUpdateTime = 0; // Track the last update time
  static int currentBrightness = 0;       // Current brightness level
  static bool isFlickering = true;        // Is the neon in flickering mode?
  static unsigned long flickerDuration = 0; // Time the flicker should last
  static unsigned long steadyStateTime = 0; // Time to maintain steady brightness

  unsigned long currentTime = millis();

  if (isFlickering) {
    // Flickering Phase
    if (currentTime - lastUpdateTime > random(50, 200)) { // Random short flicker intervals
      lastUpdateTime = currentTime;

      // Randomly turn LED off or set a brightness
      if (random(0, 10) < 3) { // 30% chance to turn off completely
        currentBrightness = 0;
      } else {
        currentBrightness = random(50, 255); // Random brightness for flicker
      }
      analogWrite(LED_PIN, currentBrightness);

      // Randomly decide when to exit flickering mode
      if (flickerDuration == 0) {
        flickerDuration = currentTime + random(3000, 5000); // Flicker for 3-5 seconds
      }
      if (currentTime > flickerDuration) {
        isFlickering = false; // End flickering mode
        steadyStateTime = currentTime + random(10000, 20000); // Stay steady for 10-20 seconds
        currentBrightness = 255; // Fully lit when steady
        analogWrite(LED_PIN, currentBrightness);
      }
    }
  } else {
    // Steady State Phase
    if (currentTime > steadyStateTime) {
      isFlickering = true; // Return to flickering mode
      flickerDuration = 0;
    }
  }
}

void neonFlickerEffect() {
  static unsigned long lastUpdateTime = 0; // Last update time
  static int currentBrightness = 0;       // Current brightness level
  static int targetBrightness = 0;        // Target brightness
  static unsigned long flickerDuration = 0; // Duration of the flicker phase
  static bool isFlickering = true;        // Whether in flickering mode

  unsigned long currentTime = millis();

  // Define update interval (shorter for flickering, longer for fading)
  const unsigned long flickerInterval = random(30, 80);  // Flicker interval
  const unsigned long fadeInterval = 20;                 // Fade step interval

  if (isFlickering) {
    // Flickering Phase
    if (currentTime - lastUpdateTime > flickerInterval) {
      lastUpdateTime = currentTime;

      // Smoothly transition to a random brightness (soft flicker)
      targetBrightness = random(50, 200); // Random brightness for soft flicker
      int step = (targetBrightness - currentBrightness) / 5; // Soft fade steps
      currentBrightness += step;

      // Ensure brightness stays within valid bounds
      currentBrightness = constrain(currentBrightness, 0, 255);
      analogWrite(LED_PIN, currentBrightness);

      // Randomly decide when to exit flickering mode
      if (flickerDuration == 0) {
        flickerDuration = currentTime + random(4000, 7000); // Flicker for 4-7 seconds
      }
      if (currentTime > flickerDuration) {
        isFlickering = false; // Exit flickering phase
        currentBrightness = 255; // Full brightness for steady phase
        analogWrite(LED_PIN, currentBrightness);
      }
    }
  } else {
    // Steady State Phase
    if (currentTime - lastUpdateTime > fadeInterval) {
      lastUpdateTime = currentTime;

      // Gradually dim and brighten softly in steady state
      targetBrightness = random(200, 255); // Target brightness in steady state
      int step = (targetBrightness - currentBrightness) / 10; // Smooth fade step
      currentBrightness += step;

      // Ensure brightness stays within valid bounds
      currentBrightness = constrain(currentBrightness, 0, 255);
      analogWrite(LED_PIN, currentBrightness);

      // Return to flickering mode after steady phase
      if (random(0, 1000) < 10) { // Small chance to re-enter flicker mode
        isFlickering = true;
        flickerDuration = 0;
      }
    }
  }
}


void adjustBrightness() {
  static unsigned long lastUpdate = 0; // Last update timestamp
  if ((millis()-lastUpdate)>brRefreshDelay)
  {
    lastUpdate = millis();
    if (brMode == BR_MODE_INC) //inc
    {
      brightness += brStep; // Increment brightness
      if (brightness >= 255) brightness = 255; // Wrap around
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
    case MODE_SCHEDULED:
      Monitor.println("Scheduled");
      break;
    case MODE_RANDOM_FADE:
      Monitor.println("Random Fade In/Out");
      break;
    case MODE_FIRE_FLICKER:
      Monitor.println("Fire Flicker");
      break;
    case MODE_NEON_FLICKER:
      Monitor.println("Neon Flicker");
      break;
    default:
      Monitor.println("Unknown");
      break;
  }
}