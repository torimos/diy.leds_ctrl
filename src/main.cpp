#include <Arduino.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>
#include <Smoothed.h>

#define BUTTON_PIN PB0
#define LED_PIN PB1
#define ADC_PIN PB3
#define TX_PIN PB4

const uint8_t gammaCorrectionTable[] = {
    0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };

SoftwareSerial Monitor(-1, TX_PIN, false);
Smoothed <uint8_t> mySensor; 

enum Mode { STATIC, FADE, FIRE };
const int modeAddress = 1;
const int fadeOutTime = 10000; // 3 seconds (3000 ms) for fade-out
const int fadeInTime = 5000;  // 1 second (1000 ms) for fade-in
Mode currentMode = STATIC;
bool lastButtonState = HIGH;
uint8_t maxBrightness = 255;
int currentBrightness = maxBrightness / 2; // Start at mid brightness
const int numADCSamples = 31; // Must be odd for median
int adcSamples[numADCSamples];
int adcValue = 0;

void saveSettings() {
  EEPROM.write(modeAddress, (uint8_t)currentMode);
}

void loadSettings() {
  currentMode = static_cast<Mode>(EEPROM.read(modeAddress));
}

void updateMaxBrightness(){
  // Take a series of readings
  for(int i=0;i<numADCSamples;i++)
  {
    adcSamples[i] = analogRead(ADC_PIN);
  }
   // Sort the array (simple bubble sort)
  for (int i = 0; i < numADCSamples - 1; i++) {
    for (int j = 0; j < numADCSamples - i - 1; j++) {
      if (adcSamples[j] > adcSamples[j + 1]) {
        int temp = adcSamples[j];
        adcSamples[j] = adcSamples[j + 1];
        adcSamples[j + 1] = temp;
      }
    }
  }
  // The median value
  adcValue = adcSamples[numADCSamples / 2];
  int br = adcValue >> 2;
  if (br < 5)
    br = 5;
  if (br > 253)
    br = 255;
  maxBrightness = br; // gammaCorrectionTable[(br >> 2)%255];
  //maxBrightness = map(adcValue, 50, 1023-50, 5, 255);
}

void debug(){
  Monitor.print(millis());
  Monitor.print(": ");
  Monitor.print("MODE: ");
  Monitor.print(currentMode);
  Monitor.print(". ADC: ");
  Monitor.print(adcValue);
  Monitor.print(". Brightness: ");
  Monitor.print(maxBrightness);
  Monitor.println();
}

void blinkLED(int times, int delayTime) {
  for (int i = 0; i < times; i++) {
    analogWrite(LED_PIN, maxBrightness);
    delay(delayTime);
    analogWrite(LED_PIN, 0);
    delay(delayTime);
  }
}

void setup() {
  delay(1000);

  mySensor.begin(SMOOTHED_EXPONENTIAL, 10);	

  Monitor.begin(9600);
  Monitor.println("LED Control V1.0");

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(ADC_PIN, INPUT_PULLUP); //adc
  pinMode(LED_PIN, OUTPUT);   // led
  pinMode(TX_PIN, OUTPUT);   // Tx
  
  loadSettings();
  updateMaxBrightness();
  blinkLED(5, 100);
  debug();
}

void loop() {
  updateMaxBrightness();
  debug();
  //analogWrite(LED_PIN, maxBrightness);
  bool buttonState = digitalRead(BUTTON_PIN);

  // Check for button release
  if (lastButtonState == LOW && buttonState == HIGH) {
    // Switch mode on button release
    currentMode = static_cast<Mode>((currentMode + 1) % 3); // Cycle through modes
    saveSettings();
    blinkLED(1+currentMode, 200);
    delay(500);
  }
  lastButtonState = buttonState;

  if (currentMode == STATIC) {
    // Static mode: Set LED to maximum brightness
    analogWrite(LED_PIN, maxBrightness);
  } 
  // else if (currentMode == FIRE) {
  //   // Fire animation: smooth flickering effect
  //   static int currentBrightness = maxBrightness / 2; // Start at mid brightness
  //   for (int i = 0; i < 50; i++) { // Adjust the loop count for duration of the effect
  //     if (digitalRead(BUTTON_PIN) == LOW && lastButtonState == HIGH) break; // Exit if mode changes
      
  //     int targetBrightness = random(maxBrightness / 2, maxBrightness); // Random target brightness
  //     int step = (targetBrightness - currentBrightness) / 10; // Smooth transition steps

  //     for (int j = 0; j < 10; j++) { // Gradually change brightness
  //       setMaxBrightness();
  //       if (digitalRead(BUTTON_PIN) == LOW && lastButtonState == HIGH) break; // Exit if mode changes
  //       currentBrightness += step;
  //       int br =  gammaCorrectionTable[constrain(currentBrightness, 0, maxBrightness)];
  //       if (br<32) br = 32;
  //       analogWrite(LED_PIN, br); // Ensure brightness is within bounds
  //       delay(30); // Smooth transition delay
  //     }
  //   }
  // }
  // else if (currentMode == FADE) {
  //   // Fade animation
  //   for (int brightness = 0; brightness <= maxBrightness; brightness++) {
  //     setMaxBrightness();
  //     if (digitalRead(BUTTON_PIN) == LOW && lastButtonState == HIGH) break; // Exit if mode changes
  //     analogWrite(LED_PIN, gammaCorrectionTable[brightness]);
  //     delay(fadeInTime / maxBrightness);
  //   }
  //   for (int brightness = maxBrightness; brightness >= 0; brightness--) {
  //     setMaxBrightness();
  //     if (digitalRead(BUTTON_PIN) == LOW && lastButtonState == HIGH) break; // Exit if mode changes
  //     analogWrite(LED_PIN, gammaCorrectionTable[brightness]);
  //     delay(fadeOutTime / maxBrightness);
  //   }
  // }
}