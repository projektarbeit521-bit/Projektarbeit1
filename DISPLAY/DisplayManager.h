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
private:
  void setFont(const GFXfont* f);
  void textLeftAligned(int x, int y, const String& s);
  void textCenteredInRect(int rx, int ry, int rw, int rh, const String& s);

  static constexpr int W = 800, H = 480;
  static constexpr int railW = 120, railPad = 8;
  static constexpr int roomBoxH = 50, dateBoxH = 30, logoBoxH = 140, chipBoxH = 40;
  static constexpr int contentPad = 20;

  int contentX = 0, contentW = 0;
  int statusX = 0, statusY = 350, statusW = 560, statusH = 100;
};
