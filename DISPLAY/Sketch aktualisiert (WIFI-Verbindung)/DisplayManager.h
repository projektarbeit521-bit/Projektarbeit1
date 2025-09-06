#pragma once
#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans24pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include "itiv_icon.h"

class DisplayManager {
public:
  void begin();
  void drawStaticFull(const String& room, const String& date,
                      const String& title, const String& name);
  void showStatusPartial(const String& msg);
  void showDatePartial (const String& date);
private:
  void setFont(const GFXfont* f);
  void textLeftAligned(int x, int y, const String& s);
  void textCenteredInRect(int rx, int ry, int rw, int rh, const String& s);

//Layout (800x480, rotation 0)
  static constexpr int W = 800, H = 480;
  static constexpr int railW = 120, railPad = 8;
  static constexpr int roomBoxH = 50, dateBoxH = 30, logoBoxH = 140, chipBoxH = 40;
  static constexpr int contentPad = 20;

// Status Window (partial)
  int contentX = 0, contentW = 0;
  int statusX = 0, statusY = 350, statusW = 560, statusH = 100;

// Date Window (partial), "x" and "y" should be multiple of 8 due to BW Configuration

  static constexpr int dateRectX = 8;          // railPad (8) -> múltiplo de 8
  static constexpr int dateRectY = railPad + roomBoxH + 4; // un poco bajo tras el "room"
  static constexpr int dateRectW = 96;         // múltiplo de 8 (96)
  static constexpr int dateRectH = 36;         // altura suficiente para 12pt

};
