#include "DisplayManager.h"
#include <GxEPD2_BW.h>

#if defined(ESP32)
static GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT>
display(GxEPD2_750_T7(/*CS=*/5, /*DC=*/17, /*RST=*/16, /*BUSY=*/4));
#endif

void DisplayManager::begin() {
  SPI.begin(18, 19, 23, -1);
  display.init(115200);
  display.setRotation(0);
  display.setFullWindow();
}

void DisplayManager::drawLayout(LayoutType layout, const String& room, const String& date,
                                const std::vector<Person>& people) {
  contentX = railW + 1;
  contentW = W - contentX - 2;

  auto drawCommon = [&]() {
    display.fillScreen(GxEPD_WHITE);
    display.drawRect(2, 2, W - 4, H - 4, GxEPD_BLACK);
    display.drawLine(railW, 2, railW, H - 2, GxEPD_BLACK);

    //Room
    setFont(&FreeSansBold24pt7b);
    textLeft(railPad, railPad + roomBoxH, room);

    //Date
    setFont(&FreeSans12pt7b);
    textLeft(dateRectX, dateRectY + dateRectH, date);

    //logo 
    {
    const int logoY = railPad + roomBoxH + dateBoxH + 10;
    const int logoW = railW - 2 * railPad;
    const int lx = railPad + (logoW - ITV_ICON_WIDTH) / 2;
    const int ly = logoY + (logoBoxH - ITV_ICON_HEIGHT) / 2;
    display.drawBitmap(lx, ly, ITV_ICON_BITS, ITV_ICON_WIDTH, ITV_ICON_HEIGHT, GxEPD_BLACK);
    }
  };

  display.setFullWindow();
  display.firstPage();
  do {
    drawCommon();
    switch (layout) {
      case LayoutType::Display1: draw1(room, date, people); break;
      case LayoutType::Display2: draw2(room, date, people); break;
      case LayoutType::Display3: draw3(room, date, people); break;
    }
  } while (display.nextPage());
}

// ===== LAYOUT 1 =====
void DisplayManager::draw1(const String&, const String&, const std::vector<Person>& p) {
  const int left = contentX + contentPad;

  setFont(&FreeSans24pt7b);
  textLeft(left, 90, p.size() ? p[0].role : "");

  setFont(&FreeSansBold24pt7b);
  textLeft(left, 140, p.size() ? p[0].name : "");

  const int statusW = 560, statusH = 100;
  const int sx = contentX + (contentW - statusW) / 2;
  const int sy = 320 - statusH / 2;

  statusRects[0] = { sx, sy, statusW, statusH };
  setFont(&FreeSansBold24pt7b);
  textCenteredInRect(sx, sy, statusW, statusH, p.size() ? p[0].status : "");
}

// ===== LAYOUT 2 =====
void DisplayManager::draw2(const String&, const String&, const std::vector<Person>& p) {
  const int left = contentX + contentPad;
  const int right = contentX + contentW - contentPad;

  const int topY = 5;
  const int bottomY = H - 5;
  const int rows = 2;
  const int rowH = (bottomY - topY) / rows;
  
  for (int i = 0; i < rows; ++i){
    const int y0 = topY + i*rowH;
    const int y1 = y0 + rowH;

    //Task
    setFont(&FreeSans12pt7b);
    const String role = (p.size() > i) ? p[i].role : String("");
    const int roleBaseY = y0 + 20;
    textLeft(left, roleBaseY, role);

    //Name
    setFont(&FreeSansBold24pt7b);
    const String name = (p.size() > i) ? p[i].name : String("");
    const int nameBaseY = roleBaseY +38;
    textLeft(left, nameBaseY, name);

    //Status
    const int statusTop = nameBaseY + 30;
    const int statusH = min(60, max(34, y1 - statusTop-6));
    statusRects[i] = {left, statusTop, contentW - 2 * contentPad, statusH};

    setFont(&FreeSansBold24pt7b);
    const String st = (p.size() > i) ? p[i].status : String("");
    textCenteredInRect(statusRects[i].x, statusRects[i].y, statusRects[i].w, statusRects[i].h, st);

    //Line to sepparate each person 
    if (i < rows - 1) {
        const int sepY = y1;
        display.drawLine(contentX + 1, sepY, contentX + contentW - 1, sepY, GxEPD_BLACK);
    }
  }
}

// ===== LAYOUT 3 =====
void DisplayManager::draw3(const String&, const String&, const std::vector<Person>& p) {
  const int left = contentX + contentPad;
  const int right = contentX + contentW - contentPad; 

  const int topY = 5;
  const int bottomY = H - 5;
  const int rows = 3;
  const int rowH = (bottomY - topY) / rows;


  for (int i = 0; i < rows; ++i) {
    //
    const int y0 = topY + i* rowH;
    const int y1 = y0 +rowH;

    //Role
    setFont(&FreeSans12pt7b);
    const String role = (p.size() > i) ? p[i].role : String("");
    const int roleBaseY = y0 + 18;
    textLeft(left, roleBaseY, role);

    //Name
    setFont(&FreeSansBold24pt7b);
    const String name = (p.size() > i) ? p[i].name : String("");
    const int nameBaseY = roleBaseY + 36;
    textLeft(left, nameBaseY, name);

    //Status variables
    const int statusTop = nameBaseY + 14;
    const int statusH = min(60, max(34, y1 - statusTop-6));

    //status placement
    statusRects[i] = { left, statusTop, contentW - 2 * contentPad, statusH };
    setFont(&FreeSansBold24pt7b);
    const String st = (p.size() > i) ? p[i].status : String("");
    textCenteredInRect(statusRects[i].x, statusRects[i].y, statusRects[i].w, statusRects[i].h, st);

    //Line to separate every person
    if (i < rows - 1){
      const int sepY = y1;
      display.drawLine(contentX + 1, sepY, contentX + contentW - 1, sepY, GxEPD_BLACK); 
    }
  }
}

// ===== Parciales =====
void DisplayManager::showStatusPartial(int idx, const String& status) {
  if (idx < 0 || idx >= 3) return;
  auto r = statusRects[idx];
  if (r.w <= 0 || r.h <= 0) return;

  display.setPartialWindow(r.x, r.y, r.w, r.h);
  display.firstPage(); do {
    display.fillRect(r.x, r.y, r.w, r.h, GxEPD_WHITE);
    setFont(&FreeSansBold24pt7b);
    textCenteredInRect(r.x, r.y, r.w, r.h, status);
  } while (display.nextPage());
}

void DisplayManager::showDatePartial(const String& date) {
  display.setPartialWindow(dateRectX, dateRectY, dateRectW, dateRectH);
  display.firstPage(); do {
    display.fillRect(dateRectX, dateRectY, dateRectW, dateRectH, GxEPD_WHITE);
    setFont(&FreeSans12pt7b);
    textCenteredInRect(dateRectX, dateRectY, dateRectW, dateRectH, date);
  } while (display.nextPage());
}

// ===== Helpers =====
void DisplayManager::setFont(const GFXfont* f) {
  display.setFont(f);
  display.setTextColor(GxEPD_BLACK);
  display.setTextWrap(false);
}
void DisplayManager::textLeft(int x, int y, const String& s) {
  display.setCursor(x, y);
  display.print(s);
}
void DisplayManager::textCenteredInRect(int rx, int ry, int rw, int rh, const String& s) {
  int16_t x1, y1; uint16_t w, h;
  display.getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
  const int cx = rx + (rw - (int)w) / 2;
  const int cy = ry + (rh + (int)h) / 2 - 2;
  display.setCursor(cx, cy);
  display.print(s);
}