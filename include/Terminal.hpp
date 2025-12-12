#pragma once

#include <cmath>
#include <cstdint>

class Terminal {
public:
  const char *const ALTBUF_ENTER = "\x1b[?1049h";
  const char *const ALTBUF_EXIT = "\x1b[?1049l";

  Terminal() {}
  virtual ~Terminal() {}

  struct Color {
    int r, g, b;
  };

  Color invert(const Color &color) {
    return {255 - color.r, 255 - color.g, 255 - color.b};
  }

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
    uint8_t r = (128 - 127 * ::cosf(n * 0.01227 * 1));
    uint8_t g = (128 - 127 * ::cosf(n * 0.01227 * 3));
    uint8_t b = (128 - 127 * ::cosf(n * 0.01227 * 5));
    Color fgColor = {r, g, b}, bgColor = {};
    setColor(fgColor, bgColor);
  }

  template <class T>
  void heatmap(T value, T min, T max) {
    if (value < min)
      value = min;
    if (value > max)
      value = max;

    double t = 0.0;
    if (max != min) {
      t = static_cast<double>(value - min) / static_cast<double>(max - min);
    } else {
      t = 0.0;
    }

    double r = 0, g = 0, b = 0;
    if (t < 0.5) {
      const double k = t / 0.5;
      r = 0.0;
      g = 255.0 * k;
      b = 255.0 * (1.0 - k);
    } else {
      const double k = (t - 0.5) / 0.5;
      r = 255.0 * k;
      g = 255.0 * (1.0 - k);
      b = 0.0;
    }

    uint8_t R = static_cast<uint8_t>(r);
    uint8_t G = static_cast<uint8_t>(g);
    uint8_t B = static_cast<uint8_t>(b);

    Color fgColor = {R, G, B}, bgColor = {};
    setColor(fgColor, bgColor);
  }
};

namespace colors {

const Terminal::Color RED = {255, 0, 0};
const Terminal::Color GREEN = {0, 255, 0};
const Terminal::Color BLUE = {0, 0, 255};

const Terminal::Color CYAN = {0, 255, 255};
const Terminal::Color MAGENTA = {255, 0, 255};
const Terminal::Color YELLOW = {255, 255, 0};

const Terminal::Color BLACK = {0, 0, 0};
const Terminal::Color WHITE = {255, 255, 255};
const Terminal::Color GRAY = {128, 128, 128};
const Terminal::Color LIGHT_GRAY = {192, 192, 192};
const Terminal::Color DARK_GRAY = {64, 64, 64};

const Terminal::Color ORANGE = {255, 165, 0};
const Terminal::Color BROWN = {165, 42, 42};
const Terminal::Color PINK = {255, 192, 203};
const Terminal::Color PURPLE = {128, 0, 128};
const Terminal::Color LIME = {50, 205, 50};
const Terminal::Color TEAL = {0, 128, 128};
const Terminal::Color NAVY = {0, 0, 128};
const Terminal::Color GOLD = {255, 215, 0};
const Terminal::Color SILVER = {192, 192, 192};

const Terminal::Color SKY_BLUE = {135, 206, 235};
const Terminal::Color DEEP_SKY_BLUE = {0, 191, 255};
const Terminal::Color TURQUOISE = {64, 224, 208};

const Terminal::Color OLIVE = {128, 128, 0};
const Terminal::Color MAROON = {128, 0, 0};
const Terminal::Color INDIGO = {75, 0, 130};
const Terminal::Color VIOLET = {238, 130, 238};

} // namespace colors
