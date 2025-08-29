#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <GxEPD2_4C.h>
#include <GxEPD2_7C.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
// For the RFID 
#include <SPI.h>
#include <MFRC522.h>
#include "GxEPD2_display_selection.h"

// Pines que usas para el RC522
#define SS_RFID   21   // NSS/SDA del RC522
#define RST_RFID  2   // RST del RC522

MFRC522 rfid(SS_RFID, RST_RFID);

String statusText = "Gone";  // Valor inicial
String currentUID = "";      // Guarda UID de la tarjeta presente

// Coordenadas de la palabra de estado (ajusta según pantalla)
int statusX = 100;
int statusY = 200;
int statusW = 400;
int statusH = 60;

// Función para actualizar solo la palabra de estado
void showStatusPartial(String msg) {
  display.setPartialWindow(statusX, statusY - 20, statusW, statusH);
  display.firstPage();
  do {
      display.fillRect(statusX, statusY - 20, statusW, statusH, GxEPD_WHITE);
      display.setFont(&FreeMonoBold18pt7b);
      display.setTextColor(GxEPD_BLACK);
      display.setCursor(statusX, statusY);
      display.print("Status: ");
      display.print(msg);
  } while (display.nextPage());
}

void setup()
{
  Serial.begin(115200);

  // Inicializar pantalla
  display.init(115200, true, 2, false);
  
  // Dibuja "Status:" fijo
  display.setRotation(0);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  display.setCursor(statusX - 150, statusY); // Ajusta según espacio
  display.print("Status:");

  // Inicializar SPI y RFID
  SPI.begin();
  rfid.PCD_Init();
  Serial.println("Acerque su tarjeta...");

  // Mostrar estado inicial
  showStatusPartial(statusText);
}

bool stateHere = false;  // inicialmente en Gone

void loop() {
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
        if (!stateHere) {
            stateHere = true;
            statusText = "Here";
            showStatusPartial(statusText);
            Serial.println("Estado cambiado a Here");
        } else {
            // Si ya estaba Here, al volver a acercar tarjeta la reseteamos a Gone
            stateHere = false;
            statusText = "Gone";
            showStatusPartial(statusText);
            Serial.println("Estado cambiado a Gone");
        }
        rfid.PICC_HaltA();
    }

  delay(500); // pequeña pausa
}