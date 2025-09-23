#include <WiFi.h>
#include <WebServer.h>
#include <time.h>
#include <vector>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <Preferences.h>  //NVS
#include <esp_sleep.h>  // Deep Sleep Mode

#include "DisplayManager.h"
#include "RFIDManager.h"

//WLAN-CONNECTION
const char* WIFI_SSID = "Luis Vásquez";
const char* WIFI_PASS = "Luisenrique89";

//NVS (Non volatile Storage)
Preferences prefs;
unsigned long lastPersistMs = 0;                 // Throttle de escrituras
const unsigned long PERSIST_COOLDOWN = 3000;     // ms


// Botón en GPIO33 (RTC). Cablear a GND (activo LOW).
#define BUTTON_PIN   33
#define BUTTON_RTC   GPIO_NUM_33

//Global declarations
DisplayManager displayManager;
RFIDManager rfidManager(21, 2);   // CS=21, RST=2

std::vector<Person> people;
String roomText = "324";
String lastDate = "";

WebServer server(80);
int layoutOverride = 0;


// Deep Sleep Modus(ajústalo a 10–15 s)
const unsigned long SLEEP_TIMEOUT_MS = 15000;

unsigned long lastActivityMs = 0;

static inline void touchActivity() { lastActivityMs = millis(); }

static void goToSleep() {
  // Configura el wake por botón LOW (EXT0)
  esp_sleep_enable_ext0_wakeup(BUTTON_RTC, 0); // 0 = LOW

  Serial.println("-> Deep Sleep");
  delay(50);
  esp_deep_sleep_start();
}


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
  Serial.print("IP: ");      Serial.println(WiFi.localIP());


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

//Layout picker
static LayoutType pickLayout(size_t n) {
  if (n <= 1) return LayoutType::Display1;
  if (n == 2) return LayoutType::Display2;
  return LayoutType::Display3; // 3 o más (mostramos 3 primeras)
}

//API HTTP (CORS + JSON)
static void sendCors() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}
static void handleOptions() { sendCors(); server.send(204); }

static void sendStateJson() {
  StaticJsonDocument<1536> doc;
  doc["room"] = roomText;
  doc["layout"] = (layoutOverride > 0) ? layoutOverride : (int)pickLayout(people.size());
  JsonArray arr = doc.createNestedArray("people");
  for (size_t i = 0; i < people.size(); ++i) {
    JsonObject o = arr.createNestedObject();
    o["name"]   = people[i].name;
    o["role"]   = people[i].role;
    o["status"] = people[i].status;
    o["uid"]    = people[i].uid;
  }
  String out; serializeJson(doc, out);
  sendCors(); server.send(200, "application/json", out);
}

static bool applyStateFromJson(const String& body, String& err) {
  StaticJsonDocument<2048> doc;
  DeserializationError e = deserializeJson(doc, body);
  if (e) { err = e.c_str(); return false; }

  if (doc.containsKey("room")) {
    roomText = (const char*)doc["room"];
  }

  if (doc.containsKey("people")) {
    std::vector<Person> compact;
    JsonArray arr = doc["people"].as<JsonArray>();

    for (JsonVariant v : arr) {
      JsonObject o = v.as<JsonObject>();
      String name   = o["name"]   | "";
      String role   = o["role"]   | "";
      String status = o["status"] | "";
      String uid    = o["uid"]    | "";

      // Vacío => no se añade (no cuenta para layout)
      if (name.length() == 0 && role.length() == 0) continue;

      // Construye persona, preservando status/uid si ya existían
      Person p;
      p.name = name;
      p.role = role;

      int prevIdx = -1;
      if (uid.length() > 0) {
        prevIdx = findPersonIndexByUID(uid);
      }
      if (prevIdx < 0) {
        for (size_t k = 0; k < people.size(); ++k) {
          if (people[k].name == name && people[k].role == role) { prevIdx = (int)k; break; }
        }
      }

      if (status.length() > 0)       p.status = status;
      else if (prevIdx >= 0)         p.status = people[prevIdx].status;
      else                           p.status = "Absent";

      p.uid = uid.length() > 0 ? uid : (prevIdx >= 0 ? people[prevIdx].uid : "");

      compact.push_back(p);
    }

    // Reemplaza: ahora people tiene 1, 2 o 3 entradas REALES
    people.swap(compact);
  }

  return true;
}

static void handleGetState() {
  touchActivity();
  sendStateJson();
}


// NVS Save { room, people[] } as JSON data
static bool saveStateToNVS() {
  StaticJsonDocument<2048> doc;
  doc["room"] = roomText;
  JsonArray arr = doc.createNestedArray("people");
  for (const auto& p : people) {
    JsonObject o = arr.createNestedObject();
    o["name"]   = p.name;
    o["role"]   = p.role;
    o["status"] = p.status;
    o["uid"]    = p.uid;
  }
  String json; serializeJson(doc, json);

  if (!prefs.begin("door", false)) {
    Serial.println("NVS: begin(write) failed");
    return false;
  }
  size_t w = prefs.putString("state", json);
  prefs.end();
  Serial.printf("NVS: saved %u bytes\n", (unsigned)w);
  return w > 0;
}

// Loads { room, people[] } from NVS (no drawing)
static bool loadStateFromNVS() {
  if (!prefs.begin("door", true)) {
    Serial.println("NVS: begin(read) failed");
    return false;
  }
  String json = prefs.getString("state", "");
  prefs.end();

  if (json.length() == 0) {
    Serial.println("NVS: empty");
    return false;
  }

  String err;
  people.clear(); 
  bool ok = applyStateFromJson(json, err);
  if (!ok) {
    Serial.print("NVS JSON invalid: "); Serial.println(err);
    people.clear();
    return false;
  }
  Serial.printf("NVS: loaded %d persons\n", (int)people.size());
  return true;
}


// Lee un tag RFID durante ~8s y devuelve { "uid": "HEX" } o 204 si no se leyó nada
static void handleReadTag() {
  touchActivity();
  // CORS
  sendCors();

  // Timeout opcional en ms: /api/read_tag?t=8000  (por defecto 8000ms)
  int timeoutMs = 8000;
  if (server.hasArg("t")) {
    int t = server.arg("t").toInt();
    if (t > 0 && t <= 30000) timeoutMs = t;
  }

  unsigned long deadline = millis() + (unsigned long)timeoutMs;

  String rawUid;
  while ((long)(deadline - millis()) > 0) {
    if (rfidManager.readUID(rawUid)) {
      // Normaliza: mayúsculas y sin separadores
      String uid = normalizeUID(rawUid);

      // Importante: liberar el bus SPI del RFID para no bloquear el EPD
      pinMode(21, OUTPUT);
      digitalWrite(21, HIGH);

      // Respuesta JSON
      String out = String("{\"uid\":\"") + uid + "\"}";
      server.send(200, "application/json", out);
      return;
    }
    delay(50);
  }

  // Nada leído dentro del timeout
  server.send(204); // No Content
}


static void handlePostState() {
  touchActivity();
  String body = server.arg("plain");
  String err;
  if (!applyStateFromJson(body, err)) {
    sendCors(); server.send(400, "text/plain", "JSON invalido: " + err); return;
  }

  // Libera el lector RFID del bus SPI antes de pintar
  pinMode(21, OUTPUT);
  digitalWrite(21, HIGH);

  lastDate = getCurrentDate();
  LayoutType lay = pickLayout(people.size());
  displayManager.drawLayout(lay, roomText, lastDate, people);

  // Mantains NVS and prepares futures partials
  saveStateToNVS();
  displayManager.primeLayout(lay, people);

  sendCors(); server.send(200, "application/json", "{\"ok\":true}");
}




static void startHttpServer() {
  // Rutas que ya tenías
  server.on("/api/state", HTTP_OPTIONS, handleOptions);
  server.on("/api/state", HTTP_GET,     handleGetState);
  server.on("/api/state", HTTP_POST,    handlePostState);

  server.on("/api/ping", HTTP_GET, []() {
    sendCors();
    server.send(200, "application/json", "{\"pong\":true}");
    
  });

  server.on("/api/read_tag", HTTP_OPTIONS, handleOptions);  //Read RFID-TAG
  server.on("/api/read_tag", HTTP_GET,     handleReadTag);  //Uploads UID to website


  server.onNotFound([]() {
    sendCors();
    server.send(404, "text/plain", "Not found");
  });

  server.begin();
}

void setup() {
  Serial.begin(115200);
  delay(100);

  // Botón con pull-up (activo LOW)
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  connectWiFiAndSyncTime();
  lastDate = getCurrentDate();

  displayManager.begin();
  rfidManager.begin();

  // NO redibujar ni limpiar al iniciar (preservar e-paper)
  // people.clear();
  // displayManager.drawLayout(pickLayout(people.size()), roomText, lastDate, people);

  // Cargar último estado persistido (si existe) y preparar partials
  if (loadStateFromNVS()) {
    LayoutType lay = pickLayout(people.size());

    // 1 FULL REFRESH al arrancar para re-sincronizar base 
    // (Evita perdida del frame Buffer al hacer reset, osea que se pierdan elementos como Datum, logo, etc)
    displayManager.drawLayout(lay, roomText, lastDate, people);

    displayManager.primeLayout(lay, people);
  }

  // mDNS SIEMPRE (independiente de loadStateFromNVS)
  if (MDNS.begin("door")) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("mDNS: http://door.local");
  }

  // Servidor HTTP SIEMPRE
  startHttpServer();
  Serial.print("HTTP ready in http://");
  Serial.println(WiFi.localIP());

  // Arrancamos ventana de actividad
  touchActivity();
}


unsigned long lastRFIDms = 0;
const unsigned long RFID_COOLDOWN = 700; // ms

void loop() {
  server.handleClient();

  String rawUid;
  if (rfidManager.readUID(rawUid)) {
    touchActivity();
    unsigned long now = millis();
    if (now - lastRFIDms > RFID_COOLDOWN) {
      lastRFIDms = now;

      String uid = normalizeUID(rawUid);
      int idx = findPersonIndexByUID(uid);

      if (idx >= 0) {
        people[idx].status = (people[idx].status == "Present") ? "Absent" : "Present";
        Serial.printf("UID %s -> %s = %s\n",
                      uid.c_str(), people[idx].name.c_str(), people[idx].status.c_str());

        // Libera RFID del bus antes de pintar
        pinMode(21, OUTPUT);
        digitalWrite(21, HIGH);

        // Refresco parcial del status
        displayManager.showStatusPartial(idx, people[idx].status);

        // Persistir con cooldown
        if (now - lastPersistMs > PERSIST_COOLDOWN) {
          saveStateToNVS();
          lastPersistMs = now;
        }
      } else {
        Serial.printf("UID %s no asignado\n", uid.c_str());
      }
    }
  }
  if (millis() - lastActivityMs > SLEEP_TIMEOUT_MS) {     //If no activity in SLEEP_TIMEOUT_MS, dann Sleep
    goToSleep();
  }
}

