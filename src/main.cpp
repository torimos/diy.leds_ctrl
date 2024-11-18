#include <Arduino.h>

#define LED_PIN PB1
#define BUTTON_PIN PB0

uint8_t mode = 0;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  analogWrite(LED_PIN, mode);
}


void loop() {
  int current_button_state = digitalRead(BUTTON_PIN);
  if (current_button_state == 0)
  {
    mode++;
    analogWrite(LED_PIN, mode % 256);
    delay(50); 
  }
}
