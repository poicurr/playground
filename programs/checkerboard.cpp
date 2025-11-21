#include <cmath>
#include <iostream>

#include <Terminal.hpp>
#ifdef _WIN32
#include <TerminalWindows.hpp>
#else
#include <TerminalLinux.hpp>
#endif

static const int TILE_SIZE = 5;

int main() {
  Terminal *terminal;
#ifdef _WIN32
  terminal = new TerminalWindows();
#else
  terminal = new TerminalLinux();
#endif
  int w = terminal->getWidth();
  int h = terminal->getHeight();
  for (int y = 1; y <= h; ++y) {
    for (int x = 1; x <= w; ++x) {
      if ((x / TILE_SIZE + y / TILE_SIZE) % 2 == 0) {
        terminal->put(x, y, ' ');
      } else {
        terminal->put(x, y, '#');
      }
    }
  }
  std::cin.get();
  delete terminal;
}
