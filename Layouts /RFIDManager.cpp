#include "RFIDManager.h"

RFIDManager::RFIDManager(int ssPin, int rstPin)
: _ssPin(ssPin), _rstPin(rstPin), _rfid(nullptr) {}

void RFIDManager::begin() {
  // Asegura SPI y deselect del SS
  SPI.begin();                     // usa el bus por defecto (ESP32: SCLK=18, MISO=19, MOSI=23)
  pinMode(_ssPin, OUTPUT);
  digitalWrite(_ssPin, HIGH);      // deselect

  // Crea e inicializa el lector
  _rfid = new MFRC522(_ssPin, _rstPin);
  _rfid->PCD_Init();

  // (Opcional) subir ganancia de antena
  _rfid->PCD_SetAntennaGain(_rfid->RxGain_max);

  Serial.println("RFID listo, acerque su tarjeta...");
}

bool RFIDManager::isCardDetected() {
  if (!_rfid) return false;

  // ¿Hay tarjeta nueva y se pudo leer?
  if (_rfid->PICC_IsNewCardPresent() && _rfid->PICC_ReadCardSerial()) {
    // Limpieza del estado del lector
    _rfid->PICC_HaltA();
    _rfid->PCD_StopCrypto1();
    return true;
  }
  return false;
}
