#define DECODE_NEC
#include <IRremote.hpp>
#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <Wire.h>
#include <TimeLib.h>
#include <DS1307RTC.h>
#include <LiquidCrystal.h>
#include <Keypad.h>
#include <GY521.h>
#include <EEPROM.h>

#define PIR_PIN 3

#define SERVO_PIN 2

#define IR_PIN 4

#define BUZZER_PIN 8

#define RFID_RESET_PIN 5
#define RFID_SS_PIN 6

#define ACCEL_ADDRESS 0x69

#define RTC_ADDRESS 0x68

#define DISPLAY_DATA_PIN_4 25
#define DISPLAY_DATA_PIN_5 27
#define DISPLAY_DATA_PIN_6 29
#define DISPLAY_DATA_PIN_7 31
#define DISPLAY_RS_PIN 22
#define DISPLAY_EN_PIN 23
#define DISPLAY_BACKLIGHT_PIN 7

#define KEYPAD_PIN_8 A8
#define KEYPAD_PIN_7 A9
#define KEYPAD_PIN_6 A10
#define KEYPAD_PIN_5 A11
#define KEYPAD_PIN_4 A12
#define KEYPAD_PIN_3 A13
#define KEYPAD_PIN_2 A14
#define KEYPAD_PIN_1 A15

#define ROWS 4
#define COLS 4
char hexaKeys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};


byte rowPins[ROWS] = {KEYPAD_PIN_1, KEYPAD_PIN_2, KEYPAD_PIN_3, KEYPAD_PIN_4};
byte colPins[COLS] = {KEYPAD_PIN_5, KEYPAD_PIN_6, KEYPAD_PIN_7, KEYPAD_PIN_8};
Keypad keypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 


bool lockStatus = true;

enum entryEvent {rfidUnlock, pinUnlock, failedPin, failedRfid, powerUp};

const String entryEventDescriptions[] = {"Unlocked by keycard at: ", "Unlocked by pin at: ", "Failed pin attempt at: ",
                                            "Invalid RFID card scanned at: ", "Powered on at: "};

int pin[4] = {1, 2, 3, 4};

int pinTrial[4] = {-1, -1, -1, -1};
int pinEntryPos = 0;
unsigned long int pinEntryResetTime = 0;

unsigned long int displayWrite = 0;

MFRC522 rfid(RFID_SS_PIN, RFID_RESET_PIN);

LiquidCrystal lcd(DISPLAY_RS_PIN, DISPLAY_EN_PIN, DISPLAY_DATA_PIN_4, DISPLAY_DATA_PIN_5, DISPLAY_DATA_PIN_6, DISPLAY_DATA_PIN_7);

Servo lockServo;

GY521 sensor(0x69);


unsigned long int lockTime = 0;
unsigned long int displayOffTime = 0;

#define NUMBER_OF_ALLOWED_CARDS 2
byte allowedCards[NUMBER_OF_ALLOWED_CARDS][4] = {
  {0xC3, 0x43, 0x1D, 0x13},
  {0x53, 0x01, 0xAA, 0x0D}
};


const int successToneNotes[] PROGMEM = {500, 201, 0, 202, 500, 203, 0, 204, 500, 205, 0, 206, 800, 150, 0, 150, 500, 500, 0, 500, 600, 1000};
const int successToneNotesLength = 11;

const int secretToneNotes[] PROGMEM = {82,117,82,117,165,117,82,117,82,117,147,117,82,117,82,117,131,117,82,117,82,117,117,117,82,117,82,117,123,117,131,117,82,117,82,117,165,117,82,117,82,117,147,117,82,117,82,117,131,117,82,117,82,117,117,705,82,117,82,117,165,117,82,117,82,117,147,117,82,117,82,117,131,117,82,117,82,117,117,117,82,117,82,117,123,117,131,117,82,117,82,117,165,117,82,117,82,117,147,117,82,117,82,117,131,117,82,117,82,117,117,705,82,117,82,117,165,117,82,117,82,117,147,117,82,117,82,117,131,117,82,117,82,117,117,117,82,117,82,117,123,117,131,117,82,117,82,117,165,117,82,117,82,117,147,117,82,117,82,117,131,117,82,117,82,117,117,705,82,117,82,117,165,117,82,117,82,117,147,117,82,117,82,117,131,117,82,117,82,117,117,117,82,117,82,117,123,117,131,117,82,117,82,117,165,117,82,117,82,117,147,117,82,117,82,117,185,88,147,88,123,88,220,88,185,88,123,88,147,88,185,88,220,88,185,88,147,88,123,88,82,117,82,117,165,117,82,117,82,117,147,117,82,117,82,117,131,117,82,117,82,117,117,117,82,117,82,117,123,117,131,117,82,117,82,117,165,117,82,117,82,117,147,117,82,117,82,117,131,117,82,117,82,117,117,705,82,117,82,117,165,117,82,117,82,117,147,117,82,117,82,117,131,117,82,117,82,117,117,117,82,117,82,117,123,117,131,117,82,117,82,117,165,117,82,117,82,117,147,117,82,117,82,117,247,88,196,88,165,88,196,88,247,88,330,88,196,88,247,88,330,88,247,88,392,88,494,88,110,117,110,117,220,117,110,117,110,117,196,117,110,117,110,117,175,117,110,117,110,117,156,117,110,117,110,117,165,117,175,117,110,117,110,117,220,117,110,117,110,117,196,117,110,117,110,117,175,117,110,117,110,117,156,705,110,117,110,117,220,117,110,117,110,117,196,117,110,117,110,117,175,117,110,117,110,117,156,117,110,117,110,117,165,117,175,117,110,117,110,117,220,117,110,117,110,117,196,117,110,117,110,117,220,88,175,88,147,88,220,88,175,88,147,88,262,88,220,88,175,88,220,88,175,88,147,88,82,117,82,117,165,117,82,117,82,117,147,117,82,117,82,117,131,117,82,117,82,117,117,117,82,117,82,117,123,117,131,117,82,117,82,117,165,117,82,117,82,117,147,117,82,117,82,117,131,117,82,117,82,117,117,705,82,117,82,117,165,117,82,117,82,117,147,117,82,117,82,117,131,117,82,117,82,117,117,117,82,117,82,117,123,117,131,117,82,117,82,117,165,117,82,117,82,117,147,117,82,117,82,117,131,117,82,117,82,117,117,705,139,117,139,117,277,117,139,117,139,117,247,117,139,117,139,117,220,117,139,117,139,117,196,117,139,117,139,117,208,117,220,117,123,117,123,117,247,117,123,117,123,117,220,117,123,117,123,117,196,117,123,117,123,117,175,705,82,117,82,117,165,117,82,117,82,117,147,117,82,117,82,117,131,117,82,117,82,117,117,117,82,117,82,117,123,117,131,117,82,117,82,117,165,117,82,117,82,117,147,117,82,117,82,117,247,88,196,88,165,88,196,88,247,88,330,88,196,88,247,88,330,88,247,88,392,88,494,88,82,117,82,117,165,117,82,117,82,117,147,117,82,117,82,117,131,117,82,117,82,117,117,117,82,117,82,117,123,117,131,117,82,117,82,117,165,117,82,117,82,117,147,117,82,117,82,117,131,117,82,117,82,117,117,705,82,117,82,117,165,117,82,117,82,117,147,117,82,117,82,117,131,117,82,117,82,117,117,117,82,117,82,117,123,117,131,117,82,117,82,117,165,117,82,117,82,117,147,117,82,117,82,117,185,88,156,88,123,88,185,88,156,88,123,88,196,88,147,88,123,88,311,88,156,88,123,88,82,117,82,117,165,117,82,117,82,117,147,117,82,117,82,117,131,117,82,117,82,117,117,117,82,117,82,117,123,117,131,117,82,117,82,117,165,117,82,117,82,117,147,117,82,117,82,117,131,117,82,117,82,117,117,705,82,117,82,117,165,117,82,117,82,117,147,117,82,117,82,117,131,117,82,117,82,117,117,117,82,117,82,117,123,117,131,117,82,117,82,117,165,117,82,117,82,117,147,117,82,117,82,117,330,88,247,88,196,88,392,88,330,88,196,88,247,88,294,88,330,88,392,88,330,88,196,88,82,117,82,117,165,117,82,117,82,117,147,117,82,117,82,117,131,117,82,117,82,117,117,117,82,117,82,117,123,117,131,117,82,117,82,117,165,117,82,117,82,117,147,117,82,117,82,117,131,117,82,117,82,117,117,705,110,117,110,117,220,117,110,117,110,117,196,117,110,117,110,117,175,117,110,117,110,117,156,117,110,117,110,117,165,117,175,117,110,117,110,117,220,117,110,117,110,117,196,117,110,117,110,117,220,88,175,88,147,88,220,88,175,88,147,88,262,88,220,88,175,88,220,88,175,88,147,88,82,117,82,117,165,117,82,117,82,117,147,117,82,117,82,117,131,117,82,117,82,117,117,117,82,117,82,117,123,117,131,117,82,117,82,117,165,117,82,117,82,117,147,117,82,117,82,117,131,117,82,117,82,117,117,705,82,117,82,117,165,117,82,117,82,117,147,117,82,117,82,117,131,117,82,117,82,117,117,117,82,117,82,117,123,117,131,117,82,117,82,117,165,117,82,117,82,117,147,117,82,117,82,117,131,117,82,117,82,117,117,705,82,117,82,117,165,117,82,117,82,117,147,117,82,117,82,117,131,117,82,117,82,117,117,117,82,117,82,117,123,117,131,117,82,117,82,117,165,117,82,117,82,117,147,117,82,117,82,117,131,117,82,117,82,117,117,705,82,117,82,117,165,117,82,117,82,117,147,117,82,117,82,117,131,117,82,117,82,117,117,117,82,117,82,117,123,117,131,117,82,117,82,117,165,117,82,117,82,117,147,117,82,117,82,117,247,88,196,88,165,88,123,88,165,88,196,88,262,88,247,88,196,88,247,88,196,88,165,88};
const int secretToneNotesLength = 680;

const int failureToneNotes[] PROGMEM = {233, 400, 185, 400, 208, 400, 156, 800};
const int failureToneNotesLength = 4;

int *currentTonePlayingNotes;
int currentTonePlayingNotesLength;
long unsigned int currentTonePlayingStartTime;
bool currentlyPlaying = false;


void log(entryEvent event){
  int addr;
  if(EEPROM[1] == 0xFF && EEPROM[0] == 0xFF){
    EEPROM[1] = 0;
    EEPROM[0] = 0x6;
    addr = 0x6;
  }else{
    addr = EEPROM[1] << 8 + EEPROM[0] + 5;
    if(addr > EEPROM.length())
      addr = 0x6;
    EEPROM[1] = addr >> 8 & 0xFF;
    EEPROM[0] = addr & 0xFF;
  }
  unsigned long time = RTC.get();
  for(int i = 0; i < 4; i++)
    EEPROM[addr+i] = time >> i*8 & 0xFF;
  EEPROM[addr+4] = event;
}

void printLog(){
  int addr = EEPROM[1] << 255 + EEPROM[0];
  int printoutCount = 0;
  while(EEPROM[addr+4] != 255){
    Serial.print("Event ");
    Serial.print(printoutCount);
    Serial.print(" - ");
    Serial.print(entryEventDescriptions[EEPROM[addr+4]]);
    time_t eventTime = EEPROM[addr] + EEPROM[addr+1] >> 8 + EEPROM[addr+2] >> 16 + EEPROM[addr+3] >> 24;
    Serial.print(hour(eventTime));
    Serial.print(":");
    Serial.print(minute(eventTime));
    Serial.print(":");
    Serial.println(second(eventTime));
    printoutCount++;
    addr -= 5;
    if(addr < 6)
      addr = 4096;
    if(printoutCount > 818)
      break;
  }

}

void setup() {
  Serial.begin(115200);
  printLog();
  lcd.begin(16, 2);
  SPI.begin();
  Wire.begin();
  while(!sensor.wakeup()){
    continue;
  }
  sensor.setAccelSensitivity(0);

  rfid.PCD_Init();
  IrReceiver.begin(IR_PIN, DISABLE_LED_FEEDBACK);
  lockServo.attach(SERVO_PIN);

  pinMode(DISPLAY_BACKLIGHT_PIN, OUTPUT);
  log(powerUp);
}


void handleTone(){
  if(currentlyPlaying){
    long unsigned int runningTimeTotal = 0;
    long unsigned int timeSinceStart = millis() - currentTonePlayingStartTime;
    bool playedTone = false;
    for(int i = 0; i < currentTonePlayingNotesLength*2; i += 2){
      if(timeSinceStart < currentTonePlayingNotes[i+1] + runningTimeTotal){
          playedTone = true;
        if(currentTonePlayingNotes[i] != 0){
          tone(BUZZER_PIN, currentTonePlayingNotes[i]);
        } 
        else{
          noTone(BUZZER_PIN);
        }
        return;
      }
      runningTimeTotal += currentTonePlayingNotes[i+1];
    }
    if(!playedTone){
      noTone(BUZZER_PIN);
      currentlyPlaying = false;      
    }
  }
}

void successTone(){
  currentTonePlayingNotes = successToneNotes;
  currentTonePlayingNotesLength = successToneNotesLength;
  currentTonePlayingStartTime = millis();
  currentlyPlaying = true;
}


void failureTone(){
  currentTonePlayingNotes = failureToneNotes;
  currentTonePlayingNotesLength = failureToneNotesLength;
  currentTonePlayingStartTime = millis();
  currentlyPlaying = true;
  }


void lock(){
  lockStatus = true;
  lockServo.write(0);
}

void unlock(){
  if(lockStatus){
    lockStatus = false;
    lockServo.write(90);
    successTone();
    lockTime = millis()+2000;
  }
}

void checkForLock(){
  if(millis() > lockTime && !lockStatus)
    lock();
}

void checkForDisplayBacklight(){
  analogWrite(DISPLAY_BACKLIGHT_PIN, constrain(map(displayOffTime - millis() + displayTimeoutTime / 2, 0, displayTimeoutTime / 2, 0, 255), 0, 255););
}

void turnOnBacklight(){
  displayOffTime = millis() + 5000;
}

void checkPIR(){
  if(digitalRead(PIR_PIN)){
    turnOnBacklight();
  }
}

int keyToNumber(char key){
  if(47 < key && key < 58){
    return key-48;
  } 
  else
    return -1;

}

void clearPin(){
  pinEntryPos = 0;
  pinTrial[0] = -1;
  pinTrial[1] = -1;
  pinTrial[2] = -1;
  pinTrial[3] = -1;
}

bool checkPin(){
  if(pinTrial[0] == pin[0] && pinTrial[1] == pin[1] && pinTrial[2] == pin[2] && pinTrial[3] == pin[3]){
    log(pinUnlock);
    unlock();
    pinEntryPos = 0;
    displayWrite = millis() + 2000;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Correct Pin");
    lcd.setCursor(0, 1);
    lcd.print("Unlocked");
    turnOnBacklight();
    clearPin();
  } 
  else{
    log(failedPin);
    failureTone();
    displayWrite = millis() + 2000;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Invalid Pin");
    turnOnBacklight();
    clearPin();
  }
}

void pinCodeEntry(int num){
  pinEntryResetTime = millis() + 2000;
  pinTrial[pinEntryPos] = num;
  if(pinEntryPos == 3){
    checkPin();
  } 
  else{
    pinEntryPos++;
  }

  if(pinTrial[0] == pinTrial[1] && pinTrial[1] == pinTrial[2] && (pinTrial[1] + pinTrial[2] + pinTrial[3]) == 0x12){
    currentTonePlayingNotes = secretToneNotes;
    currentTonePlayingNotesLength = secretToneNotesLength;
    currentTonePlayingStartTime = millis();
    currentlyPlaying = true;
  }
}

void handleKeypad(){
  char key = keypad.getKey();
  if(key){
    turnOnBacklight();
    int num = keyToNumber(key);
    if(num != -1){
      pinCodeEntry(num);
    }

  }
}

int remoteToNumber(char button){
  switch (button){
    case 0x16:
      return 0;
    case 0xC:
      return 1;
    case 0x18:
      return 2;
    case 0x5E:
      return 3;
    case 0x8:
      return 4;
    case 0x1C:
      return 5;
    case 0x5A:
      return 6;
    case 0x42:
      return 7;
    case 0x52:
      return 8;
    case 0x4A:
      return 9;
    default:
      return -1;
  }
}

void handleIR(){
    if (IrReceiver.decode()){
      int num = remoteToNumber(IrReceiver.decodedIRData.command);
      if(num != -1 && IrReceiver.decodedIRData.flags != 0x01){
        turnOnBacklight();
        pinCodeEntry(num);
      }
      IrReceiver.resume();
    }
}

void handlePinEntry(){
  if(pinEntryResetTime < millis()){
    clearPin();
  } else {
  if(pinTrial[0] != -1){
    displayWrite = millis() + 2000;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Pin: ");
    for(int i = 0; i < pinEntryPos; i++){
      lcd.print(pinTrial[i]);
      lcd.print(" ");
    }
    turnOnBacklight();
  }
  }
}

void checkAccel(){
  sensor.read();
  float magnitude = sqrt(pow(sensor.getAccelX(), 2) + pow(sensor.getAccelY(), 2) + pow(sensor.getAccelZ(), 2));
  if(magnitude > 1.25){
    turnOnBacklight();
  }
}

void handleCard(){
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()){
    for(int i = 0; i < NUMBER_OF_ALLOWED_CARDS; i++){
      if(rfid.uid.uidByte[0] == allowedCards[i][0] && rfid.uid.uidByte[1] == allowedCards[i][1] && rfid.uid.uidByte[2] == allowedCards[i][2] && rfid.uid.uidByte[3] == allowedCards[i][3]){
        log(rfidUnlock);
        displayWrite = millis() + 1000;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Valid Keycard");
        lcd.setCursor(0, 1);
        lcd.print("Unlocked");
        turnOnBacklight();
        unlock();
        }
      else{
        log(failedRfid);
        failureTone();
        displayWrite = millis() + 1000;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Invalid Keycard");
        turnOnBacklight();
        }
    }
      }
}

void displayDefaultScreen(){
 if(displayWrite < millis()){
    displayWrite = millis() + 1000;
    lcd.clear();
    lcd.print(lockStatus ? "Ready, Locked" : "Ready, Unlocked");
    lcd.setCursor(0, 1);
    lcd.print("Time: ");
    tmElements_t tm;
    RTC.read(tm);
    lcd.print(tm.Hour);
    lcd.print(":");
    lcd.print(tm.Minute);
    lcd.print(":");
    lcd.print(tm.Second);
  }
}

void loop() {
  checkForLock();
  checkForDisplayBacklight();
  checkPIR();
  checkAccel();
  handleCard();
  handleKeypad();
  handleIR();
  handlePinEntry();
  handleTone();
  displayDefaultScreen();
}
