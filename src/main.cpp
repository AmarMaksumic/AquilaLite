#include <Arduino.h>
#include <sd.hpp>

const int card_pin = 53;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println(initialize_card(card_pin) ? "SD Card is mounted!" : "SD Card is not mounted...");
  Serial.println(card_type());
  Serial.println(write_file(card_pin, "test.txt", "test message!"));
  Serial.println(read_file(card_pin, "test.txt"));
}

void loop() {
  // put your main code here, to run repeatedly:
}