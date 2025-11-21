#pragma once

#include <cmath>

class Terminal {
public:
  const char *const ALTBUF_ENTER = "\x1b[?1049h";
  const char *const ALTBUF_EXIT = "\x1b[?1049l";

  struct Color {
    int r, g, b;
  };

  virtual void setColor(int fgColor, int bgColor) = 0;
  virtual void setColor(const Color &fgColor, const Color &bgColor) = 0;
  virtual void setDefaultColor() = 0;
  virtual int getWidth() = 0;
  virtual int getHeight() = 0;
  virtual void moveUp(int amount) = 0;
  virtual void moveDown(int amount) = 0;
  virtual void moveRight(int amount) = 0;
  virtual void moveLeft(int amount) = 0;
  virtual void moveTo(int x, int y) = 0;
  virtual void moveToHead() = 0;
  virtual void put(int x, int y, char c) = 0;
  virtual void put(int x, int y, const char *s) = 0;
  virtual void clear() = 0;

  void jetColor(int n) {
    int r = (int)(128 - 127 * ::cosf(n * 0.01227 * 1));
    int g = (int)(128 - 127 * ::cosf(n * 0.01227 * 3));
    int b = (int)(128 - 127 * ::cosf(n * 0.01227 * 5));
    setColor({r, g, b}, {255 - r, 255 - g, 255 - b});
  }
};
