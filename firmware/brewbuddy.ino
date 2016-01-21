#include "math.h"
#include "QueueArray.h"

// Thermocouple Includes
#include "SparkFunMAX31855k.h"

// TFT Includes
#include "Adafruit_ILI9341.h"

// Thermocouple Variables
const uint8_t CHIP_SELECT_PIN = D3;
// SCK, MISO & MOSI are defined by Spark on A3-A5
const uint8_t VCC = D1;
const uint8_t GND = D2;
// Instantiate an instance of the SparkFunMAX31855k class
SparkFunMAX31855k probe(CHIP_SELECT_PIN, VCC, GND);

// Use hardware SPI
// cs = A2, dc = A1, rst = A0
Adafruit_ILI9341 tft = Adafruit_ILI9341(A2, A1, A0);

// Timing Variables
unsigned long previousTempMillis = 0;
unsigned long tempInterval = 5000;
unsigned long startTime = 0;
unsigned long elapsedTime;
float previousTemp = 0;
bool firstLoop = true;

//Text Size Variables
const uint8_t headingTextSize = 4;
const uint8_t subheadTextSize = 2;
const uint8_t tempTextSize = 7;
const uint8_t elapsedTimeSize = 3;
const uint8_t pixelMultiplier = 7; //Used to clear text portions of the screen

//QueueArray for the last 22 temperature readings for the TFT Graph
QueueArray<int> tempGraphArray;

const uint8_t lowTemp = 65;
const uint8_t highTemp = 165;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // Initialize TFT
  tft.begin();

  printSplash();

  delay(3000);
  tft.fillScreen(ILI9341_BLACK);

  startTime = millis();
}

void loop() {
  unsigned long currentMillis = millis();

  if(currentMillis - previousTempMillis > tempInterval) {
    previousTempMillis = currentMillis;

    float temp = readTemp();

    if (firstLoop) {
      displayTempHeading();
      displayTempHistoryHeading();
    }

    // Don't update the display if the temp hasn't changed
    if (temp != previousTemp) {
      previousTemp = temp;

      printReading(temp);
    }

    updateChart(temp);
  }

  if((currentMillis % 1000L) == 0) {
    elapsedTime = currentMillis - startTime;

    if (firstLoop) {
      displayTimeHeading();
      firstLoop = false;
    }

    displayTime(elapsedTime);
  }
}

void printSplash() {
  clearScreen();
  tft.setCursor(0, 40);
  printHeadingTextLine("BrewBuddy");
  printHeadingTextLine("v0.6");
  printSubheadingLine("Created by");
  printSubheadingLine("Brandon Satrom");
}

void clearScreen() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9341_WHITE);
}

void printHeadingTextLine(String text) {
  tft.setTextSize(headingTextSize);
  tft.println(text);
}

void printSubheadingLine(String text) {
  tft.setTextSize(subheadTextSize);
  tft.println(text);
}

float readTemp() {
  float temperature = probe.readTempF();

  if (!isnan(temperature)) {
    postTemp(temperature);

    return temperature;
  } else {
    Serial.println("Could not read temp");
    return 0;
  }
}

void postTemp(float temp) {
  String payload = "{ \"a\":" + String(temp, 2) + " }";
  Particle.publish("BatchTemp", payload);
}

void printReading(float reading) {
  tft.fillRect(0, 60, 240, tempTextSize * pixelMultiplier, ILI9341_BLACK);
  tft.setCursor(0, 60);

  if (reading <= 80) {
    tft.setTextColor(ILI9341_BLUE);
  } else if (reading > 80 && reading <= 110) {
    tft.setTextColor(ILI9341_YELLOW);
  } else if (reading > 110) {
    tft.setTextSize(ILI9341_RED);
  }

  tft.setTextSize(tempTextSize);
  tft.println(reading);

  tft.setTextColor(ILI9341_WHITE);
}

void displayTimeHeading() {
  tft.setCursor(0, 140);
  tft.setTextSize(2);
  tft.println("Elapsed Time");
}

void displayTempHeading() {
  tft.setCursor(0, 40);
  tft.setTextSize(2);
  tft.println("Wort Temp (F)");
}

void displayTempHistoryHeading() {
  tft.setCursor(0, 200);
  tft.setTextSize(2);
  tft.println("Temp History");
}

void displayTime(float elapsedTime) {
  String timeString = calcTimeToDisplay(elapsedTime); //HERE

  tft.fillRect(0, 160, 240, elapsedTimeSize * pixelMultiplier, ILI9341_BLACK);
  tft.setCursor(0, 160);
  tft.setTextSize(elapsedTimeSize);
  tft.println(timeString);
}

String calcTimeToDisplay(float elapsedTime) {
  String padZero = "0";
  String timeString;

  int timeInSec = (int)(elapsedTime / 1000L);
  int timeInMin = timeInSec / 60;
  int timeRemainder = timeInSec % 60;

  if (timeInMin < 10) {
    timeString = padZero;
  }
  timeString += String(timeInMin) + ":";

  if (timeRemainder < 10) {
    timeString += padZero;
  }
  timeString += timeRemainder;

  return timeString;
}

void updateChart(float temp) {
  int yLocation;
  int yDiff;
  int roundedTemp = (int)temp;
  int arrayLength = 0;

  if (roundedTemp < lowTemp) {
    yLocation = 315;
  } else if (roundedTemp > highTemp) {
    yLocation = 220;
  } else {
    yDiff = roundedTemp - lowTemp;

    yLocation = 320 - yDiff;
  }

  if(tempGraphArray.count() == 22) {
    tempGraphArray.dequeue();
  }
  tempGraphArray.enqueue(yLocation);

  tft.fillRect(0, 220, 240, 120, ILI9341_BLACK);

  //HERE. Need to iterate on the QueueArray for displaying on the screen
  if (tempGraphArray.count()  > 22) {
    arrayLength = 22;
  } else {
    arrayLength = tempGraphArray.count();
  }

  for (int i = 0; i < arrayLength; i++) {
    int currentLocation = tempGraphArray.dequeue();

    tft.fillCircle(10 * (i+1), currentLocation, 4, ILI9341_RED);

    //Queue the front value back onto the end of the array
    tempGraphArray.enqueue(currentLocation);
  }
}
