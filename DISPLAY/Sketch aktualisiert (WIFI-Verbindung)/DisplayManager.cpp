#include "DisplayManager.h"
#include <GxEPD2_BW.h>

#if defined(ESP32)
static GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT>
display(GxEPD2_750_T7(/*CS=*/5, /*DC=*/17, /*RST=*/16, /*BUSY=*/4));
#endif

void DisplayManager::begin() {
    SPI.begin(18, 19, 23, -1);
    display.init(115200, true, 2, false);
    display.setRotation(0);

    display.setFullWindow();
    display.firstPage();
    do { display.fillScreen(GxEPD_WHITE); } while (display.nextPage());
}

void DisplayManager::drawStaticFull(const String& room, const String& date, const String& title, const String& name) {
    contentX = railW + 1;
    contentW = W - contentX - 2;
    statusX = contentX + (contentW - statusW) / 2;

    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.drawRect(2, 2, W - 4, H - 4, GxEPD_BLACK);
        display.drawLine(railW, 2, railW, H - 2, GxEPD_BLACK);

        // Room
        setFont(&FreeSansBold24pt7b);
        textLeftAligned(railPad, railPad + roomBoxH, room);

        // Date
        setFont(&FreeSans12pt7b);
        textLeftAligned(railPad, railPad + roomBoxH + dateBoxH, date);

        // Logo
        int logoY = railPad + roomBoxH + dateBoxH + 10;
        int logoW = railW - 2 * railPad;
        int lx = railPad + (logoW - ITV_ICON_WIDTH) / 2;
        int ly = logoY + (logoBoxH - ITV_ICON_HEIGHT) / 2;
        display.drawBitmap(lx, ly, ITV_ICON_BITS, ITV_ICON_WIDTH, ITV_ICON_HEIGHT, GxEPD_BLACK);

        // CHIP box
        int chipY = H - chipBoxH - railPad;
        display.drawLine(2, chipY, railW - 1, chipY, GxEPD_BLACK);
        setFont(&FreeSans12pt7b);
        textLeftAligned(railPad, chipY + chipBoxH - 10, "CHIP");

        // Content right
        int contentLeft = contentX + contentPad;
        setFont(&FreeSans24pt7b);
        textLeftAligned(contentLeft, 80, title);

        setFont(&FreeSansBold24pt7b);
        textLeftAligned(contentLeft, 120, name);

    } while (display.nextPage());
}

void DisplayManager::showStatusPartial(const String& msg) {
    int rx = statusX;
    int ry = statusY - statusH / 2;

    display.setPartialWindow(rx, ry, statusW, statusH);
    display.firstPage();
    do {
        display.fillRect(rx, ry, statusW, statusH, GxEPD_WHITE);
        setFont(&FreeSansBold24pt7b);
        textCenteredInRect(rx, ry, statusW, statusH, msg);
    } while (display.nextPage());
}

// Partial Refresh DATE
void DisplayManager::showDatePartial(const String& date) {
  display.setPartialWindow(dateRectX, dateRectY, dateRectW, dateRectH);
  display.firstPage();
  do {
    display.fillRect(dateRectX, dateRectY, dateRectW, dateRectH, GxEPD_WHITE);
    setFont(&FreeSans12pt7b);
    // Para que se vea alineado como en el estático, centramos verticalmente en su caja.
    textCenteredInRect(dateRectX, dateRectY, dateRectW, dateRectH, date);
  } while (display.nextPage());
}
//

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
    int cx = rx + (rw - (int)w)/2;
    int cy = ry + (rh + (int)h)/2 - 2;
    display.setCursor(cx, cy);
    display.print(s);
}
