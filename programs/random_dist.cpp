#include <climits>
#include <memory>
#include <string>

#include "Terminal.hpp"
#ifdef _WIN32
#include "TerminalWindows.hpp"
#else
#include "TerminalLinux.hpp"
#endif

#include "Random.hpp"

int main(int argc, char *argv[]) {

  const int SIZE = 40;

  int samplesCount = 100000;
  if (argc > 1) {
    try {
      samplesCount = std::stoi(argv[1]);
    } catch (...) {
      std::cerr << "[error] invalid number format" << std::endl;
      return 1;
    }
  }

#ifdef _WIN32
  std::unique_ptr<Terminal> terminal = std::make_unique<TerminalWindows>();
#else
  std::unique_ptr<Terminal> terminal = std::make_unique<TerminalLinux>();
#endif

  int w = terminal->getWidth();
  int h = terminal->getHeight();

  const int topOffset = h / 2 - SIZE / 2;
  const int leftOffset = w / 2 - SIZE;

  Random r;
  std::vector<int> hist;
  hist.resize(SIZE * SIZE, 0);
  for (int i = 0; i < samplesCount; ++i) {
    ++hist[range(r, 0, SIZE - 1) * SIZE + range(r, 0, SIZE - 1)];
  }

  int min = INT_MAX, max = INT_MIN;
  for (int i = 0; i < SIZE * SIZE; ++i) {
    if (hist[i] < min)
      min = hist[i];
    if (hist[i] > max)
      max = hist[i];
  }

  for (int y = 0; y < SIZE; ++y) {
    for (int x = 0; x < SIZE; ++x) {
      int value = hist[x + SIZE * y];
      const int screenX = leftOffset + x * 2;
      terminal->heatmap(value, min, max);
      terminal->put(screenX, topOffset + y, "#");
      terminal->put(screenX + 1, topOffset + y, "#");
    }
  }

  std::cin.get();
}
