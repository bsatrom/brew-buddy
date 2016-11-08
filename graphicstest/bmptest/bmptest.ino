#include "SdFat.h"


SdFatLibSpi sd;
const uint8_t chipSelect = D3;

File myFile;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println("Starting...");

  // Initialize SdFat or print a detailed error message and halt
  // Use half speed like the native library.
  // Change to SPI_FULL_SPEED for more performance.
  if (!sd.begin(chipSelect, SPI_HALF_SPEED)) {
    sd.initErrorHalt();
  }

  // open the file for write at end like the "Native SD library"
  if (!myFile.open("test.txt", O_RDWR | O_CREAT | O_AT_END)) {
    sd.errorHalt("opening test.txt for write failed");
  }
  // if the file opened okay, write to it:
  Serial.print("Writing to test.txt...");
  myFile.println("testing 1, 2, 3.");
  myFile.printf("fileSize: %d\n", myFile.fileSize());

  // close the file:
  myFile.close();
  Serial.println("done.");

  // re-open the file for reading:
  if (!myFile.open("test.txt", O_READ)) {
    sd.errorHalt("opening test.txt for read failed");
  }
  Serial.println("test.txt content:");

  // read from the file until there's nothing else in it:
  int data;
  while ((data = myFile.read()) >= 0) {
    Serial.write(data);
  }
  // close the file:
  myFile.close();
}

void loop() {
  // nothing happens after setup
}
