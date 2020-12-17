#include <Arduino.h>
#include <sd.hpp>
#include <cam.hpp>
#include <Adafruit_BME280.h>

#define SEALEVELPRESSURE_HPA (1013.25)

const int card_pin = 9;
Adafruit_BME280 bme(8);
int i = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println(initialize_card(card_pin) ? "SD Card is mounted!" : "SD Card is not mounted...");
  Serial.println(card_type());
  Serial.println(write_file(card_pin, "test.txt", "test message!"));
  Serial.println(read_file(card_pin, "test.txt"));
  Serial.println(initialize_cam());
  Serial.println(bme.begin());  
}

void loop() {
  myCAMSaveToSDFile();

  float temperature = bme.readTemperature();
  float pressure = bme.readPressure();
  float humidity = bme.readHumidity();
  float altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);

  Serial.print("temperature:");
  Serial.print(temperature);
  Serial.print("*C   ");

  Serial.print("pressure:");
  Serial.print(pressure);
  Serial.print("hPa   ");

  Serial.print("humidity:");
  Serial.print(humidity);
  Serial.print("%   ");

  Serial.print("altitude:");
  Serial.print(altitude);
  Serial.println("m");

  String results = "Temperature: " + String(temperature) + "*C\n" + 
                    "Pressure: " + String(pressure) + "hPa\n" + 
                    "Humidity: " + String(humidity) + "%\n" + 
                    "Altitude: " + String(altitude) + "m";

  write_file(card_pin, "sensor_data_" + String(i) + ".txt", results);

  i++;
}