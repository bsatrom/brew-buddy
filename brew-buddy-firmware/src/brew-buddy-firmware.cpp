#include "application.h"
#line 1 "/Users/bsatrom/Development/brew-buddy/brew-buddy-firmware/src/brew-buddy-firmware.ino"
#include "math.h"
#include "QueueArray.h"
#include "beerBitmap.h"

// Thermocouple Includes
#include "SparkFunMAX31855k.h"

// TFT Includes
#include "Adafruit_ILI9341.h"
#include "Adafruit_ImageReader.h"

// Include for SD Card reader on TFT
#include "SdFat.h"

// App Version Constant
void setup();
void loop();
void activateBrewStage();
int setBrewMode(String command);
void printSplash();
void clearScreen();
void printHeadingTextLine(String text);
void printSubheadingLine(String text);
float readTemp();
void postTemp(float temp);
void printReading(float reading);
void displayStageName(String stagename);
void displayTimeHeading();
void displayTempHeading();
void displayTempHistoryHeading();
void displayTime(float elapsedTime);
String calcTimeToDisplay(float elapsedTime);
void updateChart(float temp);
#line 16 "/Users/bsatrom/Development/brew-buddy/brew-buddy-firmware/src/brew-buddy-firmware.ino"
#define APP_VERSION "v1.0"

SYSTEM_THREAD(ENABLED);

// Thermocouple Variables
const uint8_t CHIP_SELECT_PIN = D4;
// SCK, MISO & MOSI are defined on Particle 3rd Gen HW at A6-A8
const uint8_t VCC = D5;
const uint8_t GND = D3;
// Instantiate an instance of the SparkFunMAX31855k class
SparkFunMAX31855k probe(CHIP_SELECT_PIN, VCC, GND);

// CS Pin for SD Card on TFT
#define SD_CS D2
#define TFT_CS A2
#define TFT_DC A1
#define TFT_RST A0
// Use hardware SPI
// cs = A2, dc = A1, rst = A0
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
Adafruit_ImageReader reader; // Class w/image-reading functions
Adafruit_Image img;          // An image loaded into RAM
int32_t width = 0,           // BMP image dimensions
    height = 0;

#define TFT_SPEED 120000000

// SD Card
SdFat sd;

// #include "bmpDraw.h"

// Knock sensor pin
const uint8_t KNOCK_PIN = A4;

// Timing Variables
unsigned long previousTempMillis = 0;
unsigned long tempInterval = 5000;
unsigned long startTime = 0;
unsigned long elapsedTime;
float previousTemp = 0;

//Text Size Variables
const uint8_t headingTextSize = 4;
const uint8_t subheadTextSize = 2;
const uint8_t tempTextSize = 5;
const uint8_t elapsedTimeSize = 3;
const uint8_t pixelMultiplier = 7; //Used to clear text portions of the screen

//QueueArray for the last 22 temperature readings for the TFT Graph
QueueArray<int> tempGraphArray;
const uint8_t lowTemp = 70;
const uint8_t highTemp = 220;

//Brew Stage Variables
bool isBrewing = false;
bool isFermenting = false;

String brewStage;
String brewId;

void setup()
{
  Serial.begin(9600);

  pinMode(KNOCK_PIN, INPUT);

  //Particle Variables
  Particle.variable("brewStage", brewStage);
  Particle.variable("brewId", brewId);

  //Brew Stage cloud functions
  Particle.function("setMode", setBrewMode);

  // Initialize TFT
  tft.begin(TFT_SPEED);

  Serial.print("Initializing SD card...");
  if (!sd.begin(SD_CS))
  {
    sd.initErrorHalt();
    Serial.println("failed!");
  }
  else
  {
    Serial.println("OK!");
  }

  printSplash();

  tft.fillScreen(ILI9341_BLACK);
  printSubheadingLine("Waiting for Brew...");

  Particle.publish("Version", APP_VERSION);
  Particle.publish("Status", "Brew Buddy Online");
}

void loop()
{
  if (isFermenting)
  {
    int16_t knockVal = analogRead(KNOCK_PIN);

    if (knockVal > 80)
    {
      Serial.printlnf("Knock Val: %d", knockVal);

      printSubheadingLine("Fermentation detected!");
    }
  }
  else if (isBrewing)
  {
    unsigned long currentMillis = millis();

    if (currentMillis - previousTempMillis > tempInterval)
    {
      previousTempMillis = currentMillis;

      float temp = readTemp();

      // Don't update the display if the temp hasn't changed
      if (temp != previousTemp)
      {

        if (temp != 0)
        {
          previousTemp = temp;
        }
        printReading(previousTemp);
      }

      if (temp != 0)
      {
        updateChart(temp);
      }
    }

    if ((currentMillis % 1000L) == 0)
    {
      elapsedTime = currentMillis - startTime;

      displayTime(elapsedTime);
    }
  }
}

void activateBrewStage()
{
  startTime = millis();

  displayStageName(brewStage);
  displayTempHeading();
  displayTempHistoryHeading();
  displayTimeHeading();
}

int setBrewMode(String command)
{
  String commands[3];
  int counter = 0;
  char buffer[63]; //63 characters is the max length of the Particle string

  command.toCharArray(buffer, 63);

  char *cmd = strtok(buffer, ",");
  while (cmd != 0)
  {
    commands[counter] = cmd;
    counter++;

    cmd = strtok(NULL, ",");
  }
  free(cmd);

  brewStage = commands[2];
  brewId = commands[1];

  if (commands[0] == "brew" && !isBrewing)
  {
    isBrewing = true;
    isFermenting = false;

    clearScreen();
    activateBrewStage();

    return 1;
  }
  else if (commands[0] == "ferment")
  {
    isBrewing = false;
    isFermenting = true;

    printSubheadingLine("Waiting for");
    printSubheadingLine("Fermentation to begin...");

    return 1;
  }
  else if (commands[0] == "stop")
  {
    isBrewing = false;
    isFermenting = false;

    clearScreen();
    tft.setCursor(0, 140);
    printSubheadingLine("Waiting for Brew...");

    return 1;
  }

  return 0;
}

void printSplash()
{
  ImageReturnCode stat;
  tft.setRotation(2);

  clearScreen();

  Serial.println("Loading brew.bmp to screen...");
  stat = reader.drawBMP("brew.bmp", tft, sd, 0, 0);
  reader.printStatus(stat); // How'd we do?

  delay(2000);

  tft.setCursor(0, 40);
  printHeadingTextLine("BrewBuddy");
  printHeadingTextLine(APP_VERSION);
  printSubheadingLine("Created by");
  printSubheadingLine("Brandon Satrom");
  delay(3000);
}

void clearScreen()
{
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9341_WHITE);
}

void printHeadingTextLine(String text)
{
  tft.setTextSize(headingTextSize);
  tft.println(text);
}

void printSubheadingLine(String text)
{
  tft.setTextSize(subheadTextSize);
  tft.println(text);
}

float readTemp()
{
  float temperature = probe.readTempF();

  if (!isnan(temperature))
  {
    postTemp(temperature);

    return temperature;
  }
  else
  {
    Serial.println("Could not read temp");
    return 0;
  }
}

void postTemp(float temp)
{
  String payload = "{ \"a\":" + String(temp, 2) + ", \"b\": \"" + brewId + "\", \"c\": \"" + brewStage + "\" }";
  Particle.publish("BrewStageTemp", payload);
}

void printReading(float reading)
{
  tft.fillRect(0, 60, 240, tempTextSize * pixelMultiplier, ILI9341_BLACK);
  tft.setCursor(0, 60);

  if (reading <= 80)
  {
    tft.setTextColor(ILI9341_BLUE);
  }
  else if (reading > 80 && reading <= 110)
  {
    tft.setTextColor(ILI9341_YELLOW);
  }
  else if (reading > 110)
  {
    tft.setTextSize(ILI9341_RED);
  }

  tft.setTextSize(tempTextSize);
  tft.println(reading);

  tft.setTextColor(ILI9341_WHITE);
}

void displayStageName(String stagename)
{
  tft.setCursor(0, 10);
  tft.setTextColor(ILI9341_YELLOW);
  tft.setTextSize(2);
  tft.print("Stage: ");
  tft.println(stagename);
  tft.setTextColor(ILI9341_WHITE);
}

void displayTimeHeading()
{
  tft.setCursor(0, 120);
  tft.setTextSize(2);
  tft.println("Elapsed Time");
}

void displayTempHeading()
{
  tft.setCursor(0, 40);
  tft.setTextSize(2);
  tft.println("Wort Temp (F)");
}

void displayTempHistoryHeading()
{
  tft.setCursor(0, 170);
  tft.setTextSize(2);
  tft.println("Temp History");
}

void displayTime(float elapsedTime)
{
  String timeString = calcTimeToDisplay(elapsedTime);

  tft.fillRect(0, 140, 240, elapsedTimeSize * pixelMultiplier, ILI9341_BLACK);
  tft.setCursor(0, 140);
  tft.setTextSize(elapsedTimeSize);
  tft.println(timeString);
}

String calcTimeToDisplay(float elapsedTime)
{
  String padZero = "0";
  String timeString;

  int timeInSec = (int)(elapsedTime / 1000L);
  int timeInMin = timeInSec / 60;
  int timeRemainder = timeInSec % 60;

  if (timeInMin < 10)
  {
    timeString = padZero;
  }
  timeString += String(timeInMin) + ":";

  if (timeRemainder < 10)
  {
    timeString += padZero;
  }
  timeString += timeRemainder;

  return timeString;
}

void updateChart(float temp)
{
  int yLocation;
  int yDiff;
  int roundedTemp = (int)temp;
  int arrayLength = 0;

  if (roundedTemp < lowTemp)
  {
    yLocation = 315;
  }
  else if (roundedTemp > highTemp)
  {
    yLocation = 220;
  }
  else
  {
    yDiff = roundedTemp - lowTemp;

    yLocation = 320 - yDiff;
  }

  if (tempGraphArray.count() == 22)
  {
    tempGraphArray.dequeue();
  }
  tempGraphArray.enqueue(yLocation);

  tft.fillRect(0, 190, 240, 150, ILI9341_BLACK);

  //Iterate on the QueueArray for displaying on the screen
  if (tempGraphArray.count() > 22)
  {
    arrayLength = 22;
  }
  else
  {
    arrayLength = tempGraphArray.count();
  }

  for (int i = 0; i < arrayLength; i++)
  {
    int currentLocation = tempGraphArray.dequeue();

    tft.fillCircle(10 * (i + 1), currentLocation, 4, ILI9341_RED);

    //Queue the front value back onto the end of the array
    tempGraphArray.enqueue(currentLocation);
  }
}
