#include <Arduino.h>
#include "DisplayManager.h"
#include "RFIDManager.h"

// Objetos globales
DisplayManager displayManager;
RFIDManager rfidManager(21, 2);   // CS=21, RST=2

// Estado actual
String statusText = "STATUS";
bool stateHere = false;

void setup() {
  Serial.begin(115200);

  // Inicializar módulos
  displayManager.begin();
  rfidManager.begin();

  // Pintar UI estática
  displayManager.drawStaticFull("324", "29.08.2025", "Wissenschaftlicher Mitarbeiter", "Juan Perez");

  // Mostrar estado inicial
  displayManager.showStatusPartial(statusText);
}

void loop() {
  if (rfidManager.isCardDetected()) {
    // Alternar estado
    stateHere = !stateHere;
    statusText = stateHere ? "Here" : "Gone";
    Serial.print("Estado cambiado a "); Serial.println(statusText);

    // Mostrar en pantalla
    displayManager.showStatusPartial(statusText);
  }
  delay(200);
}
