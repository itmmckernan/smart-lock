/*
 * EEPROM Read
 *
 * Reads the value of each byte of the EEPROM and prints it
 * to the computer.
 * This example code is in the public domain.
 */
 #include <TimeLib.h>


#include <EEPROM.h>

// start reading from the first byte (address 0) of the EEPROM
int address = 0;
byte value;

void setup() {
  // initialize serial and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  printLog();

}
enum entryEvent {rfidUnlock, pinUnlock, failedPin, failedRfid, powerUp};

const String entryEventDescriptions[] = {"Unlocked by keycard at: ", "Unlocked by pin at: ", "Failed pin attempt at: ",
                                            "Invalid RFID card scanned at: ", "Powered on at: "};


void printLog(){
  int addr = EEPROM[1] << 255 + EEPROM[0];
  int printoutCount = 0;
  while(EEPROM[addr+4] != 255 && printoutCount < 818){
    Serial.print("Event ");
    Serial.print(printoutCount);
    Serial.print(" - ");
    Serial.print(entryEventDescriptions[EEPROM[addr+4]]);
    time_t eventTime = EEPROM[addr] + EEPROM[addr+1] << 8 + EEPROM[addr+2] << 16 + EEPROM[addr+3] << 24;
    Serial.print(hour(eventTime));
    Serial.print(":");
    Serial.print(minute(eventTime));
    Serial.print(":");
    Serial.print(second(eventTime));
    Serial.print(" on ");
    Serial.print(day(eventTime));
    Serial.print("/");
    Serial.print(month(eventTime));
    Serial.print("/");
    Serial.print(year(eventTime));
    Serial.print("/");
    printoutCount++;
    addr -= 5;
    if(addr < 6)
      addr = 4096;
  }

}

void loop() {
}
