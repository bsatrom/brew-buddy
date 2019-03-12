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

// Include MQTT Library to enable the Azure IoT Hub to call back to our device
#include "MQTT.h"

// App Version Constant
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
#define BATT_LOW_VOLTAGE 30

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

// Timing variables for posting readings to the cloud
unsigned long postInterval = 120000;
unsigned long previousPostMillis = 0;

// Timing variables for checking the battery state
unsigned long battInterval = 300000;
unsigned long previousBattMillis = 0;

//Text Size Variables
const uint8_t headingTextSize = 4;
const uint8_t subheadTextSize = 2;
const uint8_t tempTextSize = 5;
const uint8_t elapsedTimeSize = 3;
const uint8_t fermentationRateSize = 5;
const uint8_t pixelMultiplier = 7; //Used to clear text portions of the screen

//QueueArray for the last 22 temperature readings for the TFT Graph
QueueArray<int> tempGraphArray;
const uint8_t lowTemp = 70;
const uint8_t highTemp = 220;
float lastTemp = 0.0;

//Brew Stage Variables
bool isBrewingMode = false;
bool isFermentationMode = false;

//Variables for fermentation detection
bool isFermenting = false;
unsigned long fermentationModeStartTime = 0;
unsigned long fermentationStartTime = 0;

// Variables for fermentation rate
QueueArray<long> knockArray;
float fermentationRate = 0; // knocks per ms
unsigned long lastKnock;

String messageBase = "bb/";

String brewStage;
String brewId;

// MQTT Callback fw declaration and client
void mqttCB(char *topic, byte *payload, unsigned int length);
MQTT client("brew-buddy-hub.azure-devices.net", 8883, mqttCB);

void setup()
{
  Serial.begin(9600);

  pinMode(KNOCK_PIN, INPUT_PULLDOWN);

  //Particle Variables
  Particle.variable("brewStage", brewStage);
  Particle.variable("brewId", brewId);

  //Brew Stage cloud functions
  Particle.function("setMode", setBrewMode);
  Particle.function("checkTemp", checkTemp);
  Particle.function("checkRate", checkFermentationRate);

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

  // Check and display the battery level
  int voltage = checkVoltage();
  displayVoltage(voltage);

  //Connect to Azure MQTT Server
  /* client.connect("e00fce681290df8ab5487791", "brew-buddy-hub.azure-devices.net/e00fce681290df8ab5487791/?api-version=2018-06-30", "SharedAccessSignature sr=brew-buddy-hub.azure-devices.net&sig=RKWH%2FV8CD595YeAnXOZ8jXsSYMnWf6RiJBnzhUoxCzE%3D&skn=iothubowner&se=1552683541");
  if (client.isConnected())
  {
    Particle.publish("mqtt/status", "connected");
  }
  else
  {
    Particle.publish("mqtt/status", "failed");
  } */

  Particle.publish("Version", APP_VERSION);
  Particle.publish("Status", "Brew Buddy Online");
}

void loop()
{
  unsigned long currentMillis = millis();

  if (isFermentationMode)
  {
    int16_t knockVal = analogRead(KNOCK_PIN) / 16;

    if (knockVal >= 6)
    {
      Serial.printlnf("Knock Val: %d", knockVal);

      if (!isFermenting)
      {
        isFermenting = true;
        fermentationStartTime = millis();
        lastKnock = fermentationStartTime;

        clearScreen();
        tft.setCursor(0, 10);
        tft.setTextColor(ILI9341_YELLOW);
        tft.setTextSize(2);
        tft.print("Fermentation started");
        tft.setTextColor(ILI9341_WHITE);

        displayFermentationHeading();
        displayFermentationModeDelta();

        waitUntil(Particle.connected);
        Particle.publish(messageBase + "ferment/state", "start");
      }
      else
      {
        knockArray.push(millis() - lastKnock);
        lastKnock = millis();
        fermentationRate = getFermentationRate() / 1000.00; // # of ms between knocks

        displayFermentationRate(fermentationRate);
      }
    }
  }
  else if (isBrewingMode)
  {
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

  if (currentMillis - previousBattMillis > battInterval)
  {
    int voltage = checkVoltage();

    displayVoltage(voltage);
    Particle.publish(messageBase + "voltage", String(voltage));

    // If voltage < 30%, show a low batt message and publish message to cloud
    if (voltage < BATT_LOW_VOLTAGE)
    {
      displayLowBattAlert();
      Particle.publish(messageBase + "alert", "low-batt");
    }
  }

  if (currentMillis - previousPostMillis > postInterval)
  {
    previousPostMillis = millis();

    if (isBrewingMode)
    {
      postTemp(lastTemp);
    }
    else if (isFermentationMode)
    {
      postFermentationRate();
    }
  }
}

void resetFermentationVariables()
{
  isFermenting = false;
  fermentationModeStartTime = 0;
  fermentationStartTime = 0;
  fermentationRate = 0;
}

long getFermentationRate()
{
  long rate = 0.0;
  uint16_t arrayCount = knockArray.count();
  uint16_t sum = 0;

  for (int i = 0; i < arrayCount; i++)
  {
    uint16_t currentVal = knockArray.dequeue();
    sum += currentVal;
    knockArray.enqueue(currentVal);
  }

  rate = sum / arrayCount;

  return rate;
}

int checkFermentationRate(String args)
{
  if (isFermentationMode)
  {
    postFermentationRate();

    return 1;
  }

  return 0;
}

int checkTemp(String args)
{
  if (isBrewingMode)
  {
    float temp = readTemp();
    postTemp(temp);

    return 1;
  }

  return 0;
}

int checkVoltage()
{
  int voltage = analogRead(BATT) * 0.0011224;

  return (int)((voltage / 4.2) * 100 + 0.5);
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

  if (commands[0] == "brew" && !isBrewingMode)
  {
    isBrewingMode = true;
    isFermentationMode = false;

    clearScreen();
    activateBrewStage();

    return 1;
  }
  else if (commands[0] == "ferment")
  {
    isBrewingMode = false;
    isFermentationMode = true;
    fermentationModeStartTime = millis();

    clearScreen();
    tft.setCursor(0, 140);
    printSubheadingLine("Waiting for");
    printSubheadingLine("Fermentation...");

    System.sleep(KNOCK_PIN, CHANGE);

    return 1;
  }
  else if (commands[0] == "stop")
  {
    isBrewingMode = false;
    isFermentationMode = false;
    resetFermentationVariables();

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
    lastTemp = temperature;

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
  Particle.publish(messageBase + "brew/temp", String(temp, 2));
}

void postFermentationRate()
{
  Particle.publish(messageBase + "ferment/rate", String(fermentationRate, 2));
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
    tft.setTextColor(ILI9341_RED);
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

void displayFermentationHeading()
{
  tft.setCursor(0, 100);
  tft.setTextSize(2);
  tft.println("Fermentation Rate");
  tft.println("(in seconds)");
}

void displayFermentationModeDelta()
{
  unsigned long fermentationDelta = fermentationStartTime - fermentationModeStartTime;

  tft.setCursor(0, 40);
  tft.setTextSize(2);
  tft.println("Time to fermentation (hours)");
  tft.println(fermentationDelta / 36000000.00);
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

void displayFermentationRate(long rate)
{
  tft.fillRect(0, 80, 240, fermentationRateSize * pixelMultiplier, ILI9341_BLACK);
  tft.setCursor(0, 80);
  tft.setTextSize(fermentationRateSize);
  tft.println(rate);
}

void displayTime(float elapsedTime)
{
  String timeString = calcTimeToDisplay(elapsedTime);

  tft.fillRect(0, 140, 240, elapsedTimeSize * pixelMultiplier, ILI9341_BLACK);
  tft.setCursor(0, 140);
  tft.setTextSize(elapsedTimeSize);
  tft.println(timeString);
}

void displayVoltage(int voltage)
{
  tft.setCursor(160, 10);
  tft.setTextSize(1);
  tft.print("Batt: ");
  tft.print(voltage);
  tft.println("%");
}

void displayLowBattAlert()
{
  tft.setCursor(160, 30);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_RED);
  tft.println("LOW BATT");
  tft.setTextColor(ILI9341_WHITE);
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

void mqttCB(char *topic, byte *payload, unsigned int length)
{
  Particle.publish("mqtt/sub", topic);
}
