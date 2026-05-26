// bounce.cpp - colorful bouncing orbs demo for the playground
// Shows off Canvas + efficient present + live resize handling.

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <csignal>
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
void handleSigint(int) { g_shutdown.store(true); }

struct Orb {
  float x, y;
  float vx, vy;
  float r; // visual radius-ish (in cells)
  int hue; // for jetColor
};

int main() {
  std::signal(SIGINT, handleSigint);

#ifdef _WIN32
  auto term = std::make_unique<TerminalWindows>();
#else
  auto term = std::make_unique<TerminalLinux>();
#endif

  Canvas canvas(term.get());

  Random rng;

  // spawn some orbs
  std::vector<Orb> orbs;
  const int N = 7;
  for (int i = 0; i < N; ++i) {
    orbs.push_back(Orb{
      range(rng, 5.0f, 30.0f),
      range(rng, 5.0f, 15.0f),
      range(rng, -0.9f, 0.9f),
      range(rng, -0.7f, 0.7f),
      range(rng, 1.2f, 3.5f),
      int(range(rng, 0, 800))
    });
  }

  int frame = 0;

  while (!g_shutdown.load(std::memory_order_relaxed)) {
    canvas.resizeToTerminal(); // live resize support
    int w = canvas.width();
    int h = canvas.height();

    // "trail" by dimming previous frame (cheap fade) - dim both fg and bg for nice afterimages
    canvas.dim(0.78f, true);

    // update + draw orbs
    for (auto& o : orbs) {
      o.x += o.vx;
      o.y += o.vy;

      // bounce with a bit of energy loss
      if (o.x - o.r < 1 || o.x + o.r >= w - 1) {
        o.vx = -o.vx * 0.96f;
        o.x = std::clamp(o.x, o.r + 1, float(w) - o.r - 2);
      }
      if (o.y - o.r < 1 || o.y + o.r >= h - 1) {
        o.vy = -o.vy * 0.96f;
        o.y = std::clamp(o.y, o.r + 1, float(h) - o.r - 2);
      }

      // draw a soft-ish orb (cross + halo)
      int cx = static_cast<int>(o.x + 0.5f);
      int cy = static_cast<int>(o.y + 0.5f);
      int rad = static_cast<int>(o.r + 0.5f);

      // bright core
      Terminal::Color core = {};
      // reuse jetColor math but sample for this orb
      float t = (o.hue + frame * 1.3f) * 0.01227f;
      uint8_t cr = static_cast<uint8_t>(128 - 127 * std::cos(t * 1));
      uint8_t cg = static_cast<uint8_t>(128 - 127 * std::cos(t * 3));
      uint8_t cb = static_cast<uint8_t>(128 - 127 * std::cos(t * 5));
      core = {cr, cg, cb};

      // center + arms
      canvas.put(cx, cy, '@', core, core);
      for (int d = 1; d <= rad; ++d) {
        float a = 1.0f - float(d) / (rad + 1.0f);
        Terminal::Color halo{ uint8_t(cr * a), uint8_t(cg * a), uint8_t(cb * a) };
        canvas.put(cx + d, cy, 'o', halo, {});
        canvas.put(cx - d, cy, 'o', halo, {});
        canvas.put(cx, cy + d, 'o', halo, {});
        canvas.put(cx, cy - d, 'o', halo, {});
      }

      o.hue += 1; // slowly cycle color
    }

    // title / help
    canvas.text(2, 1, "b o u n c e", {255,255,200}, {});
    canvas.text(2, 2, "resize terminal | ctrl-c to quit", {120,120,120}, {});

    canvas.present();

    ++frame;
    std::this_thread::sleep_for(std::chrono::milliseconds(33)); // ~30fps
  }

  // Canvas dtor doesn't do anything special; Terminal dtor restores altbuf + cursor
  return 0;
}
