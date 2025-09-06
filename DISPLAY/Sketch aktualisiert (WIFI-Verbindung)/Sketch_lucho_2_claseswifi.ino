#include <WiFi.h>
#include <WiFiAP.h>
#include <WiFiClient.h>
#include <WiFiGeneric.h>
#include <WiFiMulti.h>
#include <WiFiSTA.h>
#include <WiFiScan.h>
#include <WiFiServer.h>
#include <WiFiType.h>
#include <WiFiUdp.h>

#include <Arduino.h>  //x2 included
#include <SPI.h>      //x2 included
#include <time.h>

#include "DisplayManager.h"
#include "RFIDManager.h"

//WLAN-CONNECTION

const char* WIFI_SSID = "Luis Vásquez";
const char* WIFI_PASS = "Luisenrique89";

// Objetos globales

DisplayManager displayManager;
RFIDManager rfidManager(21, 2);   // CS=21, RST=2

// Estado actual
String statusText = "STATUS";
bool stateHere = false;

// Detects day change
String lastDate = "";

// ---- Helpers fecha ----
static String getCurrentDate() {
  struct tm ti;
  if (!getLocalTime(&ti)) return lastDate.length() ? lastDate : String("??.??.????");
  char buf[11];
  strftime(buf, sizeof(buf), "%d.%m.%Y", &ti);
  return String(buf);
}

static void connectWiFiAndSyncTime() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(300);
    Serial.print(".");
  }
  Serial.println(WiFi.status() == WL_CONNECTED ? "\nWiFi OK" : "\nWiFi fallo/timeout");

  // Zona horaria Europa/Berlin (cambio horario auto)
  setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
  tzset();

  // NTP (sin offsets, los maneja TZ)
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  // Espera breve a que llegue la primera hora (opcional)
  struct tm ti;
  for (int i = 0; i < 20; ++i) {
    if (getLocalTime(&ti)) break;
    delay(200);
  }
}


void setup() {
  Serial.begin(115200);

  //Time
  connectWiFiAndSyncTime();
  lastDate = getCurrentDate();

  // Inicializar módulos
  displayManager.begin();
  rfidManager.begin();

  // Pintar UI estática
  displayManager.drawStaticFull("324", lastDate, "Wissenschaftlicher Mitarbeiter", "Juan Perez");

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

 //Date: si cambia el día, refresca solo la fecha en parcial
  String today = getCurrentDate();
  if (today != lastDate && today.indexOf('?') < 0) {   // evita partial si no hay hora válida
    lastDate = today;
    displayManager.showDatePartial(today);
    Serial.print("Fecha actualizada a "); Serial.println(today);
  }

  delay(5000); //Checks every 5 seconds
}
