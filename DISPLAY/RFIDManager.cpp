#include "RFIDManager.h"

RFIDManager::RFIDManager(int ssPin, int rstPin)
: rfid(ssPin, rstPin), ssPin(ssPin), rstPin(rstPin) {}

void RFIDManager::begin() {
    pinMode(ssPin, OUTPUT);
    digitalWrite(ssPin, HIGH); // deselect
    rfid.PCD_Init();
    Serial.println("RFID listo, acerque su tarjeta...");
}

bool RFIDManager::isCardDetected() {
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
        return true;
    }
    return false;
}
