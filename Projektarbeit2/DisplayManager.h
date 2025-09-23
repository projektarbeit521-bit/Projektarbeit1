#pragma once
#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans24pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <vector>
#include "itiv_icon.h"


struct Person {
  String name;
  String role;
  String status;
  String uid;
};

enum class LayoutType { Display1 = 1, Display2 = 2, Display3 = 3 };

class DisplayManager {
public:
  void begin();

  void drawLayout(LayoutType layout, const String& room, const String& date,
                  const std::vector<Person>& people);

  void showStatusPartial(int personIndex, const String& status);
  void showDatePartial(const String& date);
  void primeLayout(LayoutType layout, const std::vector<Person>& people);
  
private:
  void setFont(const GFXfont* f);
  void textLeft(int x, int y, const String& s);
  void textCenteredInRect(int rx, int ry, int rw, int rh, const String& s);

  void draw1(const String& room, const String& date, const std::vector<Person>& p);
  void draw2(const String& room, const String& date, const std::vector<Person>& p);
  void draw3(const String& room, const String& date, const std::vector<Person>& p);

  // Geometría
  static constexpr int W = 800, H = 480;
  static constexpr int railW = 120, railPad = 4;
  static constexpr int roomBoxH = 50, dateBoxH = 30, logoBoxH = 140, chipBoxH = 40;
  static constexpr int contentPad = 20;

  // Fecha (parcial)
  static constexpr int dateRectX = railPad;
  static constexpr int dateRectY = railPad + roomBoxH;
  static constexpr int dateRectW = railW - 2 * railPad;
  static constexpr int dateRectH = dateBoxH;

  struct Rect { int x, y, w, h; };
  Rect statusRects[3] = {};

  int contentX = 0, contentW = 0;
};
