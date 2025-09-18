#include <WebServer.h>
#include <Preferences.h>
#include <WiFi.h>
#include <time.h>
#include <vector>

#include "DisplayManager.h"
#include "RFIDManager.h"

//WLAN-CONNECTION
const char* WIFI_SSID = "CaponeraDTicuantepe";
const char* WIFI_PASS = "protosrojos";

//Global declarations
DisplayManager displayManager;
RFIDManager rfidManager(21, 2);   // CS=21, RST=2

// Personas activas para el display
std::vector<Person> people;
// Sala (texto fijo en rail izquierdo)
String roomText = "324";
// Fecha (para detec. de cambio de día)
String lastDate = "";

//RFID helpers
String normalizeUID(const String& raw) {
  String out; out.reserve(raw.length());
  for (size_t i = 0; i < raw.length(); ++i) {
    char c = raw[i];
    if (isxdigit((unsigned char)c)) out += (char)toupper((unsigned char)c);
  }
  return out;
}

int findPersonIndexByUID(const String& uid) {
  for (size_t i = 0; i < people.size(); ++i)
    if (people[i].uid.equalsIgnoreCase(uid)) return (int)i;
  return -1;
}

//Helpers date
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

// Decide layout según cantidad de personas
static LayoutType pickLayout(size_t n) {
  if (n <= 1) return LayoutType::Display1;
  if (n == 2) return LayoutType::Display2;
  return LayoutType::Display3; // 3 o más (mostramos 3 primeras)
}

void setup() {
  Serial.begin(115200);
  delay(100);

  // Tiempo
  connectWiFiAndSyncTime();
  lastDate = getCurrentDate();

  // Inicializar módulos
  displayManager.begin();
  rfidManager.begin();

  // ===== DEFINE AQUÍ tus personas =====
  // Ejemplo con 1 persona (Layout1):
  people = {
  {"Juan Perico", "Wiss. Mitarbeiter", "Absent", "6A7DE8C1"},
  {"Ana Gómez",   "HiWi",              "Absent", "0413524AD41290"},
  {"Luis Soto",   "Praktikant",        "Absent", "DEADBEEF"},
  };

  displayManager.drawLayout(pickLayout(people.size()), roomText, lastDate, people);
  
  // (Opcional) refresca status 0 en parcial por coherencia visual
  if (!people.empty()) {
    displayManager.showStatusPartial(0, people[0].status);
  }
}

unsigned long lastRFIDms = 0;
const unsigned long RFID_COOLDOWN = 700; // ms

void loop() {
  String rawUid;

  // Lee UID (una vez por pasada)
  if (rfidManager.readUID(rawUid)) {
    unsigned long now = millis();
    if (now - lastRFIDms > RFID_COOLDOWN) {
      lastRFIDms = now;

      String uid = normalizeUID(rawUid);        // o usa rawUid si ya viene limpio
      int idx = findPersonIndexByUID(uid);

      if (idx >= 0) {
        // Alterna estado
        people[idx].status = (people[idx].status == "Present") ? "Absent" : "Present";
        Serial.printf("UID %s -> %s = %s\n",
                      uid.c_str(), people[idx].name.c_str(), people[idx].status.c_str());

        // Libera el lector del bus SPI antes de pintar
        pinMode(21, OUTPUT); 
        digitalWrite(21, HIGH);

        // Parcial SOLO de esa persona
        displayManager.showStatusPartial(idx, people[idx].status);
      } else {
        Serial.printf("UID %s no asignado\n", uid.c_str());
      }
    }
  }
}


