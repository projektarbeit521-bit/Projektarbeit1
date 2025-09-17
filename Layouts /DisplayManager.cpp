#include "DisplayManager.h"

#include <SPI.h>
#include <GxEPD2_BW.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans24pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>

// ===== Pines del e-paper (ajusta si tu cableado es distinto) =====
static constexpr int EPD_CS   = 5;
static constexpr int EPD_DC   = 17;
static constexpr int EPD_RST  = 16;
static constexpr int EPD_BUSY = 4;

// Pines SPI del ESP32 (SCLK=18, MISO=19, MOSI=23)
static constexpr int SPI_SCLK = 18;
static constexpr int SPI_MISO = 19;
static constexpr int SPI_MOSI = 23;

// Instancia del display (Waveshare 7.5" b/n V2: GxEPD2_750_T7)
static GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT>
display(GxEPD2_750_T7(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

void DisplayManager::begin() {
  // Inicia SPI con pines explícitos
  SPI.begin(SPI_SCLK, SPI_MISO, SPI_MOSI, -1);

  // init(baud, initial_refresh, pulses, enable_greyscale)
  display.init(115200, true, 2, false);
  display.setRotation(0);

  // Limpieza inicial
  display.setFullWindow();
  display.firstPage();
  do { display.fillScreen(GxEPD_WHITE); } while (display.nextPage());
}

void DisplayManager::drawStaticFull(const String& room, const String& date,
                                    const String& title, const String& name) {
  // Calcula zonas derivadas
  contentX = railW + 1;
  contentW = W - contentX - 2;
  statusX  = contentX + (contentW - statusW) / 2;

  display.setFullWindow();
  display.firstPage();
  do {
    // Marco general
    display.fillScreen(GxEPD_WHITE);
    display.drawRect(2, 2, W - 4, H - 4, GxEPD_BLACK);
    display.drawLine(railW, 2, railW, H - 2, GxEPD_BLACK);

    // Room (izquierda)
    setFont(&FreeSansBold24pt7b);
    textLeftAligned(railPad, railPad + roomBoxH, room);

    // Date (izquierda, sobre su rectángulo)
    setFont(&FreeSans12pt7b);
    // Para que coincida con el parcial usamos el mismo rectángulo visual:
    textLeftAligned(dateRectX, dateRectY + dateRectH, date);

    // Logo en el rail
    {
      const int logoY = railPad + roomBoxH + dateBoxH + 10;
      const int logoW = railW - 2 * railPad;
      const int lx = railPad + (logoW - ITV_ICON_WIDTH) / 2;
      const int ly = logoY + (logoBoxH - ITV_ICON_HEIGHT) / 2;
      display.drawBitmap(lx, ly, ITV_ICON_BITS, ITV_ICON_WIDTH, ITV_ICON_HEIGHT, GxEPD_BLACK);
    }

    // Caja CHIP en la parte baja del rail
    {
      const int chipY = H - chipBoxH - railPad;
      display.drawLine(2, chipY, railW - 1, chipY, GxEPD_BLACK);
      setFont(&FreeSans12pt7b);
      textLeftAligned(railPad, chipY + chipBoxH - 10, "CHIP");
    }

    // Contenido derecho
    const int contentLeft = contentX + contentPad;

    setFont(&FreeSans24pt7b);
    textLeftAligned(contentLeft, 80, title);

    setFont(&FreeSansBold24pt7b);
    textLeftAligned(contentLeft, 120, name);

    // (El área de status se actualiza en parcial)
  } while (display.nextPage());
}

void DisplayManager::showStatusPartial(const String& msg) {
  // Rect de status centrado horizontalmente dentro del panel derecho
  const int rx = statusX;
  const int ry = statusY - statusH / 2;

  display.setPartialWindow(rx, ry, statusW, statusH);
  display.firstPage();
  do {
    display.fillRect(rx, ry, statusW, statusH, GxEPD_WHITE);
    setFont(&FreeSansBold24pt7b);
    textCenteredInRect(rx, ry, statusW, statusH, msg);
  } while (display.nextPage());
}

void DisplayManager::showDatePartial(const String& date) {
  // Actualiza SOLO la fecha en el rail izquierdo
  display.setPartialWindow(dateRectX, dateRectY, dateRectW, dateRectH);
  display.firstPage();
  do {
    display.fillRect(dateRectX, dateRectY, dateRectW, dateRectH, GxEPD_WHITE);
    setFont(&FreeSans12pt7b);
    textCenteredInRect(dateRectX, dateRectY, dateRectW, dateRectH, date);
  } while (display.nextPage());
}

// ===== Helpers de texto =====
void DisplayManager::setFont(const GFXfont* f) {
  display.setFont(f);
  display.setTextColor(GxEPD_BLACK);
  display.setTextWrap(false);
}

void DisplayManager::textLeftAligned(int x, int y, const String& s) {
  display.setCursor(x, y);
  display.print(s);
}

void DisplayManager::textCenteredInRect(int rx, int ry, int rw, int rh, const String& s) {
  int16_t x1, y1; uint16_t w, h;
  display.getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
  const int cx = rx + (rw - (int)w) / 2;
  const int cy = ry + (rh + (int)h) / 2 - 2; // ajuste fino vertical
  display.setCursor(cx, cy);
  display.print(s);
}
