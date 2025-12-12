#include <array>
#include <string_view>
#include <utility>

template <class K, class V, size_t N>
class Matcher {
public:
  constexpr Matcher(std::array<std::pair<K, V>, N> entries, V defaultValue)
      : m_entries(entries), m_default(defaultValue) {}

  template <size_t M = N>
  constexpr auto add(K key, V value) const {
    std::array<std::pair<K, V>, M + 1> next{};
    for (size_t i = 0; i < N; ++i) {
      next[i] = m_entries[i];
    }
    next[M] = {key, value};
    return Matcher<K, V, M + 1>{next, m_default};
  }

  constexpr auto orElse(V value) const {
    return Matcher<K, V, N>(m_entries, value);
  }

  constexpr V match(K key) const {
    for (const auto &e : m_entries) {
      if (e.first == key) {
        return e.second;
      }
    }
    return m_default;
  }

private:
  std::array<std::pair<K, V>, N> m_entries;
  V m_default;
};

template <class K, class V>
constexpr auto makeMatcher(V defaultValue = {}) {
  return Matcher<K, V, 0>{{}, defaultValue};
}

int main() {
  enum class Animal { Dog, Cat, Bird };
  constexpr auto matcher = makeMatcher<Animal, std::string_view>("Undefined")
                               .add(Animal::Dog, "dog")
                               .add(Animal::Cat, "cat")
                               .add(Animal::Bird, "bird");
  static_assert(matcher.match(Animal::Dog) == "dog");
  static_assert(matcher.match(Animal::Cat) == "cat");
  static_assert(matcher.match(Animal::Bird) == "bird");
}
