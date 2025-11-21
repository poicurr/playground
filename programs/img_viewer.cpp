#include <cstdint>
#include <iostream>
#include <memory>

#ifdef _WIN32
#include <TerminalWindows.hpp>
#else
#include <TerminalLinux.hpp>
#endif

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define CLAMP(v, a, b) (MAX(MIN(v, b), a))

static constexpr int RAMP_LEVELS = 10;

static constexpr char DENSITY_RAMP[] = " .:-=+*#%@";

inline double encodedLuma01(uint8_t r, uint8_t g, uint8_t b) {
  const double R = r / 255.0;
  const double G = g / 255.0;
  const double B = b / 255.0;
  return 0.2126 * R + 0.7152 * G + 0.0722 * B;
}

inline char glyphFromRgb(uint8_t r, uint8_t g, uint8_t b) {
  const double yPrime = CLAMP(encodedLuma01(r, g, b), 0.0, 1.0);
  const double t = 1.0 - yPrime;
  int idx = static_cast<int>(t * (RAMP_LEVELS - 1) + 0.5);
  idx = CLAMP(idx, 0, RAMP_LEVELS - 1);
  return DENSITY_RAMP[static_cast<size_t>(idx)];
}

inline char glyphFromRgbInverted(uint8_t r, uint8_t g, uint8_t b) {
  const double yPrime = CLAMP(encodedLuma01(r, g, b), 0.0, 1.0);
  int idx = static_cast<int>(yPrime * (RAMP_LEVELS - 1) + 0.5);
  idx = CLAMP(idx, 0, RAMP_LEVELS - 1);
  return DENSITY_RAMP[static_cast<size_t>(idx)];
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "[Usage] " << argv[0]
              << " 'path/to/image.png(.jpg, .bmp, etc...)'\n";
    return 1;
  }

#ifdef _WIN32
  std::unique_ptr<Terminal> terminal = std::make_unique<TerminalWindows>();
#else
  std::unique_ptr<Terminal> terminal = std::make_unique<TerminalLinux>();
#endif

  const int termW = terminal->getWidth();
  const int termH = terminal->getHeight();
  if (termW <= 0 || termH <= 0) {
    std::cerr << "Terminal size is invalid.\n";
    return 1;
  }

  int w, h, ch;
  stbi_uc *image = stbi_load(argv[1], &w, &h, &ch, 3);
  if (!image) {
    std::cerr << "failed to load image: " << argv[1] << ".\n";
    return 1;
  }

  if (w <= 0 || h <= 0) {
    std::cerr << "image size is invalid.\n";
    return 1;
  }

  for (int ty = 1; ty <= termH; ++ty) {
    int y0 = (static_cast<long long>(ty - 1) * h) / termH;
    int y1 = (static_cast<long long>(ty) * h) / termH;
    y1 = MAX(y1, MIN(h, y0 + 1));

    for (int tx = 1; tx <= termW; ++tx) {
      int x0 = (static_cast<long long>(tx - 1) * w) / termW;
      int x1 = (static_cast<long long>(tx) * w) / termW;
      x1 = MAX(x1, MIN(w, x0 + 1));

      long long sumR = 0, sumG = 0, sumB = 0;
      const int bw = x1 - x0;
      const int bh = y1 - y0;
      const int count = bw * bh;

      for (int y = y0; y < y1; ++y) {
        for (int x = x0; x < x1; ++x) {
          size_t offset = (y * w + x) * ch;
          sumR += image[offset + 0];
          sumG += image[offset + 1];
          sumB += image[offset + 2];
        }
      }

      const int r = static_cast<int>(sumR / count);
      const int g = static_cast<int>(sumG / count);
      const int b = static_cast<int>(sumB / count);

      terminal->setColor({r, g, b}, {});
      terminal->put(tx, ty, glyphFromRgbInverted(r, g, b));
    }
  }

  std::cin.get();
  stbi_image_free(image);
  return 0;
}
