
#include <iostream>
#include <windows.h>

namespace {
constexpr const char *const ALTBUF_ENTER = "\x1b[?1049h";
constexpr const char *const ALTBUF_EXIT = "\x1b[?1049l";
} // namespace

class TerminalAltBufferGuard {
public:
  TerminalAltBufferGuard() {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(h, &mode)) {
      m_prevMode = mode;
      SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
      m_enabled = true;
    }
    std::cout << ALTBUF_ENTER;
    std::cout.flush();
  }

  ~TerminalAltBufferGuard() {
    std::cout << ALTBUF_EXIT;
    std::cout.flush();
    if (m_enabled) {
      HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
      SetConsoleMode(h, m_prevMode);
    }
  }

  TerminalAltBufferGuard(const TerminalAltBufferGuard &) = delete;
  TerminalAltBufferGuard &operator=(const TerminalAltBufferGuard &) = delete;

private:
  DWORD m_prevMode{0};
  bool m_enabled{false};
};

int main() {
  TerminalAltBufferGuard guard; // 代替画面へ
  std::cout << "This is alternate screen.\n";
  std::cout << "\x1b[2J\x1b[H"; // クリア＋ホーム
  std::cout << "Rendering...\n";
  std::cout.flush();
  Sleep(1500);
  // guardがスコープアウトで元画面に復帰
}
