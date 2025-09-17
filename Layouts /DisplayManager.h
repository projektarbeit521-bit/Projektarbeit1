#pragma once
#include <Arduino.h>
#include "itiv_icon.h"

// Forward declaration para evitar incluir las fuentes en el .h
class GFXfont;

class DisplayManager {
public:
  void begin();
  void drawStaticFull(const String& room, const String& date,
                      const String& title, const String& name);
  void showStatusPartial(const String& msg);
  void showDatePartial(const String& date);

private:
  // Helpers de texto
  void setFont(const GFXfont* f);
  void textLeftAligned(int x, int y, const String& s);
  void textCenteredInRect(int rx, int ry, int rw, int rh, const String& s);

  // Dimensiones base (7.5" 800x480)
  static constexpr int W = 800, H = 480;

  // Rail izquierdo y layout
  static constexpr int railW   = 120;
  static constexpr int railPad = 8;

  static constexpr int roomBoxH = 50;
  static constexpr int dateBoxH = 30;
  static constexpr int logoBoxH = 140;
  static constexpr int chipBoxH = 40;

  static constexpr int contentPad = 20;

  // Área de la fecha (parcial)
  static constexpr int dateRectX = railPad;
  static constexpr int dateRectY = railPad + roomBoxH;
  static constexpr int dateRectW = railW - 2 * railPad;
  static constexpr int dateRectH = dateBoxH;

  // Área de status (parcial, en el panel derecho)
  int contentX = 0, contentW = 0;
  int statusX = 0;
  int statusY = 350;
  int statusW = 560;
  int statusH = 100;
};
