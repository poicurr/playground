#include <cmath>
#include <iostream>

#include <Terminal.hpp>
#ifdef _WIN32
#include <TerminalWindows.hpp>
#else
#include <TerminalLinux.hpp>
#endif

int main() {
  std::unique_ptr<Terminal> terminal;
#ifdef _WIN32
  terminal = std::make_unique<TerminalWindows>();
#else
  terminal = std::make_unique<TerminalLinux>();
#endif
  int w = terminal->getWidth();
  int h = terminal->getHeight();
  float len = w > h ? w : h;
  float scale = 10.0f / len;
  for (int x = 1; x <= w; ++x) {
    for (int y = 1; y <= h; ++y) {
      terminal->jetColor(255 * (std::sinf(static_cast<float>(x) * scale) +
                                std::sinf(static_cast<float>(y) * scale)));
      terminal->put(x, y, ' ');
    }
  }
  std::cin.get();
}
