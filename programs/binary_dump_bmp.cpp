#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

#include "Bitmap.hpp"

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <binary file>" << std::endl;
    return 1;
  }

  const char *filename = argv[1];
  constexpr int WIDTH = 512;

  std::ifstream file(filename, std::ios::binary);
  if (!file) {
    std::cerr << "Failed to open file: " << filename << std::endl;
    return 1;
  }

  std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(file), {});

  size_t pixelCount = buffer.size() / 3;
  size_t height =
      static_cast<size_t>(std::ceil(pixelCount / static_cast<double>(WIDTH)));

  Bitmap bitmap(WIDTH, height);
  Color *image = bitmap.data();

  for (size_t i = 0; i < pixelCount; ++i) {
    image[i].r = buffer[i * 3 + 0];
    image[i].g = buffer[i * 3 + 1];
    image[i].b = buffer[i * 3 + 2];
  }

  bitmap.save("./out.bmp");

  return 0;
}
