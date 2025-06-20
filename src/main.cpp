#include <Arduino.h>

#include <MD_MAX72xx.h>
#include <SPI.h>
#include <EEPROM.h>
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define ROWS 8
#define DELAY 1000
#define CLK_PIN 12
#define CS_PIN 11
#define DATA_PIN 10
#define CLOSE_IN 4
#define OPEN_IN 3
#define HALF 2
#define SAVE 5

auto mx =
    MD_MAX72XX(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
int currentDevice = 3;
int currentRow = 7;
unsigned long lastMillis = 0;
unsigned long lastPress = 0;
unsigned long lastSave = 0;
bool isClosing = false;
bool isOpening = false;
bool isHalf = false;

void setRowFull(const int device, const int row) {
  mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
  mx.setColumn(device, row, 0b11111111);
  mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
}
void setRowEmpty(const int device, const int row) {
  mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
  mx.setColumn(device, row, 0b00000000);
  mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
}
bool isFirstRow() { return currentRow == 0 && currentDevice == 0; }
bool isLastRow() {
  return currentRow == ROWS - 1 && currentDevice == MAX_DEVICES - 1;
}
void opening() {
  if (millis() < DELAY + lastMillis) {
    return;
  }
  setRowFull(currentDevice, currentRow);

  if (isLastRow()) {
    isOpening = false;
    return;
  }
  if (currentRow <= ROWS - 1) {
    currentRow++;
  }

  lastMillis = millis();

  if (currentRow == ROWS) {
    currentRow = 0;
    currentDevice++;
  }
}
void closing() {
  if (millis() < DELAY + lastMillis) {
    return;
  }
  if (currentRow % 2 == 1 || !isHalf) {
    setRowEmpty(currentDevice, currentRow);
  }
  if (isFirstRow()) {
    isClosing = false;
    return;
  }
  if (currentRow >= 0) {
    currentRow--;
  }
  lastMillis = millis();
  if (currentRow == -1) {
    currentRow = ROWS - 1;
    currentDevice--;
  }
}
void half() {
  if (!isHalf) {
    for (int i = currentRow + 1; i < ROWS; i++) {
      setRowEmpty(currentDevice, i);
    }
    for (int i = currentDevice + 1; i < MAX_DEVICES; i++) {
      for (int j = 0; j < ROWS; j++) {
        setRowEmpty(i, j);
      }
    }
    return;
  }
  for (int i = 0; i < MAX_DEVICES; i++) {
    for (int j = 0; j < ROWS; j++) {
      if (j % 2 == 0) {
        setRowFull(i, j);
      }
    }
  }
}
void save() {
  if (millis() < DELAY + lastSave) {
    return;
  }
  int valueToSave = currentDevice * ROWS + currentRow;
  EEPROM.write(0, valueToSave);
  int isHalfOpen = 0;
  if(isHalf) {
    isHalfOpen = 1;
  }
  EEPROM.write(1, isHalfOpen);
}
void setup() {
  Serial.begin(57600);
  pinMode(CLOSE_IN, INPUT_PULLUP);
  pinMode(OPEN_IN, INPUT_PULLUP);
  pinMode(HALF, INPUT_PULLUP);
  pinMode(SAVE, INPUT_PULLUP);
  mx.begin();
  mx.control(MD_MAX72XX::INTENSITY, 0);
  int saveFull = EEPROM.read(0);
  int saveHalf = EEPROM.read(1);
  isHalf = (saveHalf == 1);
  int rowsToLight = saveFull % ROWS;
  int devicesToLight = saveFull / ROWS;
  for (int i = 0; i < devicesToLight; i++) {
    for (int j = 0; j < ROWS; j++) {
      setRowFull(i, j);
    }
  }
  for(int i = 0; i < rowsToLight; i++) {
    setRowFull(devicesToLight, i);
  }
}

void loop() {
  if(digitalRead(SAVE) == LOW) {
    save();
  }
  if (digitalRead(CLOSE_IN) == HIGH) {
    isClosing = false;
  }
  if (digitalRead(OPEN_IN) == HIGH) {
    isOpening = false;
  }

  if (digitalRead(CLOSE_IN) == LOW && !isFirstRow()) {
    isOpening = false;
    isClosing = true;
  }

  if (digitalRead(OPEN_IN) == LOW && !isLastRow()) {
    isOpening = true;
    isClosing = false;
  }
  if (digitalRead(HALF) == LOW && millis() > 1000 + lastPress) {
    isHalf = !isHalf;
    half();
    lastPress = millis();
  }

  if (isClosing) {
    closing();
  }
  if (isOpening) {
    opening();
  }
}