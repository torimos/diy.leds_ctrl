#include <Arduino.h>
#include <SoftwareSerial.h>
#include <SimpleKalmanFilter.h>
//#include <EEPROM.h>

#define BUTTON_PIN PB0
#define LED_PIN PB1
#define ADC_PIN PB3
#define TX_PIN PB4

const uint8_t gammaCorrectionTable[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
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

uint8_t maxBrightness = 0;
SoftwareSerial Monitor(-1, TX_PIN, false);
// https://github.com/denyssene/SimpleKalmanFilter/blob/master/README.md#basic-usage
SimpleKalmanFilter kf(1, 1, 0.1);

enum Mode { STATIC, FADE, FIRE };
Mode currentMode = STATIC;
bool lastButtonState = HIGH;
const int modeAddress = 0;
const int fadeOutTime = 3000; // 3 seconds (3000 ms) for fade-out
const int fadeInTime = 1000;  // 1 second (1000 ms) for fade-in

void loadSettings() {
  //currentMode = static_cast<Mode>(EEPROM.read(modeAddress));
}

void saveSettings() {
  //EEPROM.write(modeAddress, currentMode);
}

void setBrightness(){
  for(int i=0;i<50;i++)
  {
    int adc_value = ((analogRead(ADC_PIN)+analogRead(ADC_PIN)+analogRead(ADC_PIN)+analogRead(ADC_PIN)+analogRead(ADC_PIN))/5);
    int adc_fvalue = ((int)kf.updateEstimate(adc_value))>>2;
    maxBrightness = gammaCorrectionTable[adc_fvalue];
  }
}

void debug(){
  Monitor.print(millis());
  Monitor.print(": ");
  Monitor.print("MODE: ");
  Monitor.print(currentMode);
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
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(ADC_PIN, INPUT_PULLUP); //adc
  pinMode(LED_PIN, OUTPUT);   // led
  pinMode(TX_PIN, OUTPUT);   // Tx
  
  //loadSettings();
  Monitor.begin(9600);
  delay(500);
  debug();
  blinkLED(1+currentMode, 200);
  Monitor.println("LED Control V1.0");

  setBrightness();
  analogWrite(LED_PIN, maxBrightness);
}

void loop() {

    bool buttonState = digitalRead(BUTTON_PIN);

    // Check for button release
    if (lastButtonState == LOW && buttonState == HIGH) {
      // Switch mode on button release
      currentMode = static_cast<Mode>((currentMode + 1) % 3); // Cycle through modes
      debug();
      //saveSettings();
      blinkLED(1+currentMode, 200);
    }
    lastButtonState = buttonState;

    if (currentMode == STATIC) {
      // Static mode: Set LED to maximum brightness
      analogWrite(LED_PIN, maxBrightness);
      Monitor.print(millis());
      Monitor.println(" Ping");
      delay(1000);
    } 
    else if (currentMode == FIRE) {
      // Fire animation: smooth flickering effect
      static int currentBrightness = maxBrightness / 2; // Start at mid brightness

      for (int i = 0; i < 50; i++) { // Adjust the loop count for duration of the effect
        if (digitalRead(BUTTON_PIN) == LOW && lastButtonState == HIGH) break; // Exit if mode changes
        int targetBrightness = random(maxBrightness / 2, maxBrightness); // Random target brightness
        int step = (targetBrightness - currentBrightness) / 10; // Smooth transition steps

        for (int j = 0; j < 10; j++) { // Gradually change brightness
          if (digitalRead(BUTTON_PIN) == LOW && lastButtonState == HIGH) break; // Exit if mode changes
          currentBrightness += step;
          analogWrite(LED_PIN, constrain(currentBrightness, 0, maxBrightness)); // Ensure brightness is within bounds
          delay(15); // Smooth transition delay
        }
      }
    }
    else if (currentMode == FADE) {
      // Fade animation
      for (int brightness = 0; brightness <= maxBrightness; brightness++) {
        if (digitalRead(BUTTON_PIN) == LOW && lastButtonState == HIGH) break; // Exit if mode changes
        analogWrite(LED_PIN, brightness);
        delay(10);
      }
      for (int brightness = maxBrightness; brightness >= 0; brightness--) {
        if (digitalRead(BUTTON_PIN) == LOW && lastButtonState == HIGH) break; // Exit if mode changes
        analogWrite(LED_PIN, brightness);
        delay(10);
      }
    }
}