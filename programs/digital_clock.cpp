#include <atomic>
#include <csignal>
#include <cmath>
#include <ctime>
#include <thread>

#include "Canvas.hpp"
#ifdef _WIN32
#include "TerminalWindows.hpp"
#else
#include "TerminalLinux.hpp"
#endif

std::string getTimeString() {
  std::time_t t = std::time(nullptr);
  char mbstr[100];
  std::strftime(mbstr, sizeof(mbstr), "%H%M%S", std::localtime(&t));
  return mbstr;
}

constexpr int ctoi(const char c) { return c - '0'; }

struct Vec2 {
  int x, y;
};

// Canvas-based 7-segment display
class SevenSegmentDisplay {
  Canvas* canvas = nullptr;
  bool segment[7]{};
  Terminal::Color fgColor{255, 255, 255};
  Terminal::Color bgColor{0, 0, 0};

  using Segment = bool[7];
  const Segment number[10]{{1, 1, 1, 1, 1, 1, 0}, {0, 1, 1, 0, 0, 0, 0},
                           {1, 1, 0, 1, 1, 0, 1}, {1, 1, 1, 1, 0, 0, 1},
                           {0, 1, 1, 0, 0, 1, 1}, {1, 0, 1, 1, 0, 1, 1},
                           {1, 0, 1, 1, 1, 1, 1}, {1, 1, 1, 0, 0, 0, 0},
                           {1, 1, 1, 1, 1, 1, 1}, {1, 1, 1, 1, 0, 1, 1}};

public:
  SevenSegmentDisplay() = default;
  explicit SevenSegmentDisplay(Canvas* c) : canvas{c} {
    setNumber(number[0]);
  }
  void setCanvas(Canvas* c) { canvas = c; }

  void setColor(const Terminal::Color& fg, const Terminal::Color& bg = {}) {
    fgColor = fg;
    bgColor = bg;
  }

  void setNumber(int n) { setNumber(number[n]); }

  void putNumber(int x, int y, int scale) {
    putSegmentA(x, y, scale);
    putSegmentB(x, y, scale);
    putSegmentC(x, y, scale);
    putSegmentD(x, y, scale);
    putSegmentE(x, y, scale);
    putSegmentF(x, y, scale);
    putSegmentG(x, y, scale);
  }

  void putColon(int x, int y, int scale) {
    sweepY(x + scale + 1, y + 1, scale - 1);
    sweepY(x + scale + 1, y + scale + 2, scale - 1);
  }

private:
  void paint(int x, int y) {
    if (canvas) canvas->put(x, y, ' ', fgColor, fgColor);
  }

  void sweepX(int x, int y, int length) {
    for (int i = x; i <= x + length; ++i) paint(i, y);
  }
  void sweepY(int x, int y, int length) {
    for (int i = y; i <= y + length; ++i) paint(x, i);
  }

  void setNumber(const bool data[7]) {
    for (int i = 0; i < 7; ++i) segment[i] = data[i];
  }

  inline void putSegmentA(int x, int y, int scale) {
    if (segment[0]) sweepX(x, y, scale + 1);
  }
  inline void putSegmentB(int x, int y, int scale) {
    if (segment[1]) sweepY(x + scale + 1, y, scale + 1);
  }
  inline void putSegmentC(int x, int y, int scale) {
    if (segment[2]) sweepY(x + scale + 1, y + scale + 1, scale + 1);
  }
  inline void putSegmentD(int x, int y, int scale) {
    if (segment[3]) sweepX(x, y + 2 * (scale + 1), scale + 1);
  }
  inline void putSegmentE(int x, int y, int scale) {
    if (segment[4]) sweepY(x, y + scale + 1, scale + 1);
  }
  inline void putSegmentF(int x, int y, int scale) {
    if (segment[5]) sweepY(x, y, scale + 1);
  }
  inline void putSegmentG(int x, int y, int scale) {
    if (segment[6]) sweepX(x, y + scale + 1, scale + 1);
  }
};

class DigitalClock {
  Canvas* canvas;
  SevenSegmentDisplay seg[6];
  Vec2 pos;
  int scale;

public:
  explicit DigitalClock(Canvas* c) : canvas(c) {
    for (auto &s : seg) s.setCanvas(c);
    pos.x = 1;
    pos.y = 1;
    scale = 2;
  }

  void setPosition(int x, int y) { pos = {x, y}; }
  int getX() { return pos.x; }
  int getY() { return pos.y; }
  void setScale(int s) { scale = s; }

  void setColor(const Terminal::Color& fg, const Terminal::Color& bg = {}) {
    for (auto &s : seg) s.setColor(fg, bg);
  }

  void setTime(int d1, int d2, int d3, int d4, int d5, int d6) {
    seg[0].setNumber(d1);
    seg[1].setNumber(d2);
    seg[2].setNumber(d3);
    seg[3].setNumber(d4);
    seg[4].setNumber(d5);
    seg[5].setNumber(d6);
  }

  void draw() {
    int posX = pos.x;
    int posY = pos.y;

    for (int i = 0; i < 6; ++i) {
      seg[i].putNumber(posX, posY, scale);
      posX += (scale + 2) + 1;
      if (i % 2 != 0 && i != 5) {
        seg[i].putColon(posX, posY, scale);
        posX += (scale + 2) + 1;
      }
    }
  }
};

std::atomic<bool> g_shutdown{false};

void handleSigint(int) { g_shutdown.store(true, std::memory_order_relaxed); }

int main() {
  std::signal(SIGINT, handleSigint);

#ifdef _WIN32
  std::unique_ptr<Terminal> term = std::make_unique<TerminalWindows>();
#else
  std::unique_ptr<Terminal> term = std::make_unique<TerminalLinux>();
#endif

  Canvas canvas(term.get());
  DigitalClock clock(&canvas);

  const int SCALE = 2;
  const int NUM_SEGMENTS = 8;
  const int CLOCK_WIDTH = (SCALE + 3) * NUM_SEGMENTS - 1;
  const int CLOCK_HEIGHT = SCALE * 2 + 3;

  clock.setScale(SCALE);
  Vec2 velocity{2, 1};
  uint32_t frame = 0;

  while (!g_shutdown.load(std::memory_order_relaxed)) {
    canvas.resizeToTerminal();
    canvas.clear(Cell{' ', {25, 25, 25}, {}});

    std::string timeStr = getTimeString();
    int d1 = ctoi(timeStr[0]);
    int d2 = ctoi(timeStr[1]);
    int d3 = ctoi(timeStr[2]);
    int d4 = ctoi(timeStr[3]);
    int d5 = ctoi(timeStr[4]);
    int d6 = ctoi(timeStr[5]);

    // Same jetColor math as before
    float t = (frame * 0.01227f);
    uint8_t r = static_cast<uint8_t>(128 - 127 * std::cos(t * 1));
    uint8_t g = static_cast<uint8_t>(128 - 127 * std::cos(t * 3));
    uint8_t b = static_cast<uint8_t>(128 - 127 * std::cos(t * 5));
    Terminal::Color digitColor{r, g, b};

    clock.setColor(digitColor, {});
    clock.setTime(d1, d2, d3, d4, d5, d6);

    int x = clock.getX();
    int y = clock.getY();
    int newX = x + velocity.x;
    int newY = y + velocity.y;

    int w = canvas.width();
    int h = canvas.height();

    if (newX <= 1 || newX + CLOCK_WIDTH >= w) velocity.x *= -1;
    if (newY <= 1 || newY + CLOCK_HEIGHT >= h) velocity.y *= -1;

    clock.setPosition(newX, newY);
    clock.draw();

    canvas.text(2, 1, "digital clock (canvas)", {100, 100, 100}, {});

    canvas.present();
    ++frame;
    std::this_thread::sleep_for(std::chrono::milliseconds(70));
  }
}
