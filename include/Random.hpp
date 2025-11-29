#pragma once

#include <cstdint>
#include <random>

// PCG32
class Random {
public:
  explicit Random(std::uint64_t seed = std::random_device{}()) {
    m_state = 0;
    m_inc = (seed << 1u) | 1u;
    next();
    m_state += seed;
    next();
  }

  uint32_t operator()() { return next(); }

  static constexpr uint32_t min() { return 0; }
  static constexpr uint32_t max() { return UINT32_MAX; }

private:
  uint64_t m_state;
  uint64_t m_inc;

  uint32_t next() {
    uint64_t oldState = m_state;
    m_state = oldState * 6364136223846793005ULL + m_inc;

    uint32_t xorshifted =
        static_cast<uint32_t>(((oldState >> 18u) ^ oldState) >> 27u);
    uint32_t rot = oldState >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31u));
  }
};

// static Random g_rand;
//
// template <class T>
// T rand() {
//   return g_rand() / static_cast<T>(g_rand.max());
// }
