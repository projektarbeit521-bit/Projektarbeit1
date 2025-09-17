#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>

class RFIDManager {
public:
  // ssPin = SDA/SS del MFRC522, rstPin = RST del MFRC522
  RFIDManager(int ssPin, int rstPin);

  void begin();
  bool isCardDetected();   // true cuando hay tarjeta nueva y se leyó el serial

private:
  int _ssPin;
  int _rstPin;
  MFRC522* _rfid;          // se instancia en begin()
};