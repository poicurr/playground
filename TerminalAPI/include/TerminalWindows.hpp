#pragma once

#include "Terminal.hpp"

#include <windows.h>

#include <iostream>

class TerminalWindows : public Terminal {
public:
  TerminalWindows() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(h, &m_info);
    DWORD mode = 0;
    if (GetConsoleMode(h, &mode)) {
      m_prevMode = mode;
      SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
      m_enabled = true;
    }
    std::cout << ALTBUF_ENTER << "\033[?25l";
    std::cout.flush();
  }

  virtual ~TerminalWindows() {
    std::cout << "\033[?25h" << ALTBUF_EXIT;
    std::cout.flush();
    if (m_enabled) {
      HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
      SetConsoleMode(h, m_prevMode);
    }
  }

  void setColor(int fgColor, int bgColor) {
    std::cout << "\033[38;5;" << fgColor << "m";
    std::cout << "\033[48;5;" << bgColor << "m";
  }
  void setColor(const Color &fgColor, const Color &bgColor) {
    std::cout << "\033[38;2;" << fgColor.r << ";" << fgColor.g << ";"
              << fgColor.b << "m";
    std::cout << "\033[48;2;" << bgColor.r << ";" << bgColor.g << ";"
              << bgColor.b << "m";
  }
  void setDefaultColor() {
    std::cout << "\033[39m" << std::flush;
    std::cout << "\033[49m" << std::flush;
  }

  int getWidth() { return m_info.srWindow.Right - m_info.srWindow.Left + 1; }
  int getHeight() { return m_info.srWindow.Bottom - m_info.srWindow.Top + 1; }

  void moveUp(int amount) {
    std::cout << "\033[" << amount << "A" << std::flush;
  }
  void moveDown(int amount) {
    std::cout << "\033[" << amount << "B" << std::flush;
  }
  void moveRight(int amount) {
    std::cout << "\033[" << amount << "C" << std::flush;
  }
  void moveLeft(int amount) {
    std::cout << "\033[" << amount << "D" << std::flush;
  }
  void moveTo(int x, int y) {
    std::cout << "\033[" << y << ";" << x << "H" << std::flush;
  }
  void moveToHead() { moveTo(1, 1); }
  void put(int x, int y, char c) {
    moveTo(x, y);
    std::cout << c << std::flush;
  }
  void put(int x, int y, const char *s) {
    moveTo(x, y);
    std::cout << s << std::flush;
  }
  void clear() { std::cout << "\033[3J" << std::flush; }

private:
  bool m_enabled{false};
  DWORD m_prevMode{0};
  CONSOLE_SCREEN_BUFFER_INFO m_info;
};
