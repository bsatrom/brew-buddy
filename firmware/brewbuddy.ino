#include "SPI.h"

// Thermocouple Includes
#include "SparkFunMAX31855k.h"

// TFT Includes
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9340.h"

// Thermocouple Variables
const uint8_t CHIP_SELECT_PIN = 10;
// SCK & MISO are defined by Arduino
const uint8_t VCC = 5;
const uint8_t GND = 6;
// Instantiate an instance of the SparkFunMAX31855k class
SparkFunMAX31855k probe(CHIP_SELECT_PIN, VCC, GND);

// TFT Variables
#define _sclk 13
#define _miso 12
#define _mosi 11
#define _cs 7
#define _dc 9
#define _rst 8
// Use hardware SPI
Adafruit_ILI9340 tft = Adafruit_ILI9340(_cs, _dc, _rst);

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // Initialize TFT
  tft.begin();

  printSplash();

  delay(3000);
}

void loop() {
  printReading(readTemp());
  delay(5000);
}

void printSplash() {
  clearScreen();
  tft.setCursor(0, 40);
  printHeadingTextLine("BrewBuddy");
  printHeadingTextLine("v0.5");
  printSubheadingLine("Created by");
  printSubheadingLine("Brandon Satrom");
}

void clearScreen() {
  tft.fillScreen(ILI9340_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9340_WHITE);
}

void printHeadingTextLine(String text) {
  tft.setTextSize(4);
  tft.println(text);
}

void printSubheadingLine(String text) {
  tft.setTextSize(2);
  tft.println(text);
}

float readTemp() {
  float temperature = probe.readTempF();

  if (!isnan(temperature)) {
    return temperature;
  } else {
    Serial.println("Could not read temp");
    return 0;
  }
}

void printReading(float reading) {
  tft.fillScreen(ILI9340_BLACK);
  tft.setCursor(0, 160);
  tft.setTextColor(ILI9340_WHITE);
  tft.setTextSize(5);
  tft.println(reading);
}
