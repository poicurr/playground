#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace robovoice {

constexpr int SAMPLE_RATE = 48000;
constexpr double PI = 3.14159265358979323846;
constexpr double MASTER_GAIN = 0.25;

struct SyllableParams {
  double frequencyHz = 220.0;
  double durationSec = 0.12;
  double vowelShape = 0.0;
  double noiseAmount = 0.0;
  double bend = 0.0;
};

static uint32_t hashString(std::string_view text) {
  uint32_t hash = 2166136261u;
  for (unsigned char c : text) {
    hash ^= c;
    hash *= 16777619u;
  }
  return hash;
}

static double clamp(double value, double minValue, double maxValue) {
  return std::max(minValue, std::min(value, maxValue));
}

static double envelope(double t, double duration) {
  const double attack = 0.015;
  const double release = 0.035;

  if (t < attack) {
    return t / attack;
  }

  if (t > duration - release) {
    return clamp((duration - t) / release, 0.0, 1.0);
  }

  return 1.0;
}

static double softClip(double x) { return std::tanh(x); }

static double squareLike(double phase) {
  const double s = std::sin(phase);
  return s >= 0.0 ? 1.0 : -1.0;
}

static double sawLike(double phase) {
  const double normalized = std::fmod(phase / (2.0 * PI), 1.0);
  return 2.0 * normalized - 1.0;
}

static std::vector<SyllableParams> makeSyllables(std::string_view text) {
  std::vector<SyllableParams> result;
  result.reserve(text.size());

  uint32_t seed = hashString(text);
  std::mt19937 rng(seed);

  for (unsigned char c : text) {
    if (std::isspace(c) != 0) {
      SyllableParams pause;
      pause.frequencyHz = 0.0;
      pause.durationSec = 0.08;
      result.push_back(pause);
      continue;
    }

    const uint32_t v = static_cast<uint32_t>(c) * 2654435761u;

    SyllableParams s;
    s.frequencyHz = 170.0 + static_cast<double>((v >> 3) % 420);
    s.durationSec = 0.07 + static_cast<double>((v >> 11) % 90) / 1000.0;
    s.vowelShape = static_cast<double>((v >> 19) % 100) / 100.0;
    s.noiseAmount = static_cast<double>((v >> 25) % 30) / 100.0;
    s.bend = (static_cast<double>((v >> 28) % 9) - 4.0) * 0.015;

    result.push_back(s);
  }

  return result;
}

static void appendSyllable(std::vector<float> &samples, const SyllableParams &s,
                           std::mt19937 &rng) {
  const int nSamples = static_cast<int>(s.durationSec * SAMPLE_RATE);

  if (s.frequencyHz <= 0.0) {
    samples.insert(samples.end(), nSamples, 0.0f);
    return;
  }

  std::uniform_real_distribution<double> noiseDist(-1.0, 1.0);

  double phase = 0.0;

  for (int i = 0; i < nSamples; ++i) {
    const double t = static_cast<double>(i) / SAMPLE_RATE;
    const double x = static_cast<double>(i) / std::max(1, nSamples - 1);

    const double frequency =
        s.frequencyHz * (1.0 + s.bend * std::sin(2.0 * PI * x));

    phase += 2.0 * PI * frequency / SAMPLE_RATE;

    const double sine = std::sin(phase);
    const double square = squareLike(phase);
    const double saw = sawLike(phase);

    const double roboticCarrier = 0.55 * sine + 0.25 * square + 0.20 * saw;

    const double formant1 = std::sin(phase * (1.8 + s.vowelShape * 1.7)) * 0.25;
    const double formant2 = std::sin(phase * (2.7 + s.vowelShape * 2.2)) * 0.14;

    const double noise = noiseDist(rng) * s.noiseAmount;

    const double amp = envelope(t, s.durationSec);
    const double value =
        softClip((roboticCarrier + formant1 + formant2 + noise) * amp);

    samples.push_back(static_cast<float>(value * MASTER_GAIN));
  }
}

static std::vector<float> synthesize(std::string_view text) {
  std::vector<float> samples;
  auto syllables = makeSyllables(text);

  std::mt19937 rng(hashString(text));

  for (const auto &s : syllables) {
    appendSyllable(samples, s, rng);

    const int gapSamples = static_cast<int>(0.012 * SAMPLE_RATE);
    samples.insert(samples.end(), gapSamples, 0.0f);
  }

  return samples;
}

static void writeU16(std::ofstream &out, uint16_t value) {
  out.put(static_cast<char>(value & 0xff));
  out.put(static_cast<char>((value >> 8) & 0xff));
}

static void writeU32(std::ofstream &out, uint32_t value) {
  out.put(static_cast<char>(value & 0xff));
  out.put(static_cast<char>((value >> 8) & 0xff));
  out.put(static_cast<char>((value >> 16) & 0xff));
  out.put(static_cast<char>((value >> 24) & 0xff));
}

static bool writeWav(const std::filesystem::path &path,
                     std::span<const float> samples) {
  std::ofstream out(path, std::ios::binary);
  if (!out) {
    return false;
  }

  constexpr uint16_t NUM_CHANNELS = 1;
  constexpr uint16_t BITS_PER_SAMPLE = 16;
  constexpr uint16_t AUDIO_FORMAT_PCM = 1;
  constexpr uint32_t BYTE_RATE =
      SAMPLE_RATE * NUM_CHANNELS * BITS_PER_SAMPLE / 8;
  constexpr uint16_t BLOCK_ALIGN = NUM_CHANNELS * BITS_PER_SAMPLE / 8;

  const uint32_t dataSize =
      static_cast<uint32_t>(samples.size() * sizeof(int16_t));
  const uint32_t riffSize = 36 + dataSize;

  out.write("RIFF", 4);
  writeU32(out, riffSize);
  out.write("WAVE", 4);

  out.write("fmt ", 4);
  writeU32(out, 16);
  writeU16(out, AUDIO_FORMAT_PCM);
  writeU16(out, NUM_CHANNELS);
  writeU32(out, SAMPLE_RATE);
  writeU32(out, BYTE_RATE);
  writeU16(out, BLOCK_ALIGN);
  writeU16(out, BITS_PER_SAMPLE);

  out.write("data", 4);
  writeU32(out, dataSize);

  for (float sample : samples) {
    const double clamped = clamp(sample, -1.0f, 1.0f);
    const auto pcm = static_cast<int16_t>(std::lrint(clamped * 32767.0));
    writeU16(out, static_cast<uint16_t>(pcm));
  }

  return true;
}

static int playWav(const std::filesystem::path &path) {
#ifdef _WIN32
  const std::string command = "powershell -NoProfile -Command "
                              "\"(New-Object Media.SoundPlayer '" +
                              path.string() + "').PlaySync()\"";
  return std::system(command.c_str());
#else
  {
    const std::string command = "paplay \"" + path.string() + "\"";
    const int code = std::system(command.c_str());
    if (code == 0) {
      return code;
    }
  }

  {
    const std::string command = "aplay \"" + path.string() + "\"";
    const int code = std::system(command.c_str());
    if (code == 0) {
      return code;
    }
  }

  {
    const std::string command =
        "ffplay -nodisp -autoexit -loglevel quiet \"" + path.string() + "\"";
    return std::system(command.c_str());
  }
#endif
}

} // namespace robovoice

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "usage: robovoice \"text to speak\"\n";
    return 1;
  }

  std::string text;
  for (int i = 1; i < argc; ++i) {
    if (!text.empty()) {
      text += ' ';
    }
    text += argv[i];
  }

  const auto samples = robovoice::synthesize(text);

  const auto outputPath =
      std::filesystem::temp_directory_path() / "robovoice_output.wav";

  if (!robovoice::writeWav(outputPath, samples)) {
    std::cerr << "failed to write wav: " << outputPath << "\n";
    return 1;
  }

  std::cout << "[robovoice] speaking: " << text << "\n";
  return robovoice::playWav(outputPath);
}
