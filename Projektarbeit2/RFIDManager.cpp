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
    return (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial());
}

// Nuevo método: lee el UID como String HEX mayúsculas
bool RFIDManager::readUID(String &uidOut) {
    if (!(rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial())) {
        return false;
    }

    uidOut = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
        if (rfid.uid.uidByte[i] < 0x10) uidOut += "0";
        uidOut += String(rfid.uid.uidByte[i], HEX);
    }
    uidOut.toUpperCase();

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    return true;
}
