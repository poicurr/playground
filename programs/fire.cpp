// fire.cpp — classic terminal fire effect using Canvas
// Heat propagation + RGB palette. Looks good in most modern terminals.

#include <algorithm>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <thread>
#include <vector>

#include "Canvas.hpp"
#include "Random.hpp"

#ifdef _WIN32
#include "TerminalWindows.hpp"
#else
#include "TerminalLinux.hpp"
#endif

std::atomic<bool> g_shutdown{false};
void handleSigint(int) { g_shutdown.store(true, std::memory_order_relaxed); }

// Simple but effective fire color ramp (0 = black, 255 = hot)
inline Terminal::Color fireColor(uint8_t t) {
  if (t < 16)  return {0, 0, 0};
  if (t < 48) {
    // deep red / dark
    uint8_t v = (t - 16) * 2;
    return {v, 0, 0};
  }
  if (t < 96) {
    // red → orange
    uint8_t r = 255;
    uint8_t g = uint8_t((t - 48) * 2.8);
    return {r, g, 0};
  }
  if (t < 160) {
    // orange → yellow
    uint8_t r = 255;
    uint8_t g = 120 + uint8_t((t - 96) * 1.6);
    uint8_t b = uint8_t((t - 120) * 0.8);
    return {r, std::min<uint8_t>(255, g), b};
  }
  // hot yellow-white
  uint8_t u = (t - 160) * 2 + 40;
  return {255, std::min<uint8_t>(255, 180 + u), std::min<uint8_t>(255, 60 + u / 2)};
}

// Character ramp for extra texture on top of colors
inline char fireChar(uint8_t t) {
  if (t < 20)  return ' ';
  if (t < 55)  return '.';
  if (t < 95)  return ':';
  if (t < 135) return '^';
  if (t < 175) return '*';
  if (t < 210) return '#';
  return (t & 1) ? '@' : '%';
}

int main() {
  std::signal(SIGINT, handleSigint);

#ifdef _WIN32
  auto term = std::make_unique<TerminalWindows>();
#else
  auto term = std::make_unique<TerminalLinux>();
#endif

  Canvas canvas(term.get());
  Random rng;

  std::vector<uint8_t> heat;
  int simW = 0, simH = 0;

  auto resizeFire = [&](int w, int h) {
    heat.assign(size_t(w) * h, 0u);
    // seed the bottom row with heat
    for (int x = 0; x < w; ++x) {
      heat[size_t(h - 1) * w + x] = 140 + (rng() % 90);
    }
    simW = w;
    simH = h;
  };

  [[maybe_unused]] int frame = 0;

  while (!g_shutdown.load(std::memory_order_relaxed)) {
    canvas.resizeToTerminal();
    int w = canvas.width();
    int h = canvas.height();

    if (w != simW || h != simH || simW == 0) {
      resizeFire(w, h);
    }

    // === HEAT SIMULATION (bottom-up) ===
    // Each cell takes heat from below with slight horizontal drift + cooling
    for (int y = 0; y < h - 1; ++y) {
      for (int x = 0; x < w; ++x) {
        // pick one of the three cells below (with wind bias)
        int bias = (rng() % 5) - 2;           // -2..+2
        int nx = x + bias;
        if (nx < 0) nx = 0;
        if (nx >= w) nx = w - 1;

        uint8_t val = heat[size_t(y + 1) * w + nx];

        // random cooling (the soul of the effect)
        int cool = (rng() % 6);
        if (val > cool) val -= cool;
        else val = 0;

        // extra decay near the top for smoother fade
        if (y < h / 4) {
          if (val > 1) val -= 1;
        }

        heat[size_t(y) * w + x] = val;
      }
    }

    // Re-heat the bottom source (campfire style)
    for (int x = 0; x < w; ++x) {
      uint8_t base = 170 + (rng() % 70);
      // occasional tall flames
      if ((rng() % 9) == 0) base = 230 + (rng() % 26);
      // occasional gaps
      if ((rng() % 17) == 0) base = 90 + (rng() % 50);

      heat[size_t(h - 1) * w + x] = base;
    }

    // === RENDER TO CANVAS ===
    // Black background + fire colors. Using bg for volume + char for detail.
    for (int y = 0; y < h; ++y) {
      for (int x = 0; x < w; ++x) {
        uint8_t t = heat[size_t(y) * w + x];
        if (t < 12) {
          canvas.cell(x, y) = Cell{' ', {}, {}};
          continue;
        }

        Terminal::Color c = fireColor(t);
        char ch = fireChar(t);

        // For the main body use strong background, darker fg for contrast
        // On the very hot tips we put bright chars
        if (t > 200) {
          canvas.cell(x, y) = Cell{ch, {255, 255, 220}, c};
        } else if (t > 140) {
          canvas.cell(x, y) = Cell{ch, {220, 140, 40}, c};
        } else {
          // cooler parts — colored bg, very dark fg
          Terminal::Color dark{ uint8_t(c.r / 5), uint8_t(c.g / 6), 0 };
          canvas.cell(x, y) = Cell{' ', dark, c};
        }
      }
    }

    // UI overlay (will be on top because we wrote it after the fire)
    canvas.text(3, 1, "F I R E", {255, 230, 180}, {});
    canvas.text(3, 2, "classic heat propagation  |  resize terminal  |  ctrl-c to quit", {90, 90, 90}, {});

    // subtle ember line at very bottom
    if (h > 3) {
      for (int x = 0; x < w; x += 3) {
        if (heat[size_t(h-1)*w + x] > 200) {
          canvas.cell(x, h-1) = Cell{'*', {255,255,200}, {120,40,0}};
        }
      }
    }

    canvas.present();

    ++frame;
    // Fire looks nice around 20-30 fps; higher is also fine
    std::this_thread::sleep_for(std::chrono::milliseconds(28));
  }

  return 0;
}
