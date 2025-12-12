#include <iostream>
#include <memory>

#include <Terminal.hpp>
#ifdef _WIN32
#include <TerminalWindows.hpp>
#else
#include <TerminalLinux.hpp>
#endif

static const int TILE_SIZE = 5;

int main() {
#ifdef _WIN32
  std::unique_ptr<Terminal> terminal = std::make_unique<TerminalWindows>();
#else
  std::unique_ptr<Terminal> terminal = std::make_unique<TerminalLinux>();
#endif

  int w = terminal->getWidth();
  int h = terminal->getHeight();
  for (int y = 1; y <= h; ++y) {
    for (int x = 1; x <= w; ++x) {
      if ((x / TILE_SIZE + y / TILE_SIZE) % 2 == 0) {
        terminal->setColor(colors::BLACK, colors::WHITE);
      } else {
        terminal->setColor(colors::WHITE, colors::BLACK);
      }
      terminal->put(x, y, ' ');
    }
  }
  std::cin.get();
}
