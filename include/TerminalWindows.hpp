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
    m_hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    std::cout << ALTBUF_ENTER << "\033[?25l";
    clear();
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

  void setColor(int fgColor, int bgColor) override {
    std::cout << "\033[38;5;" << fgColor << "m\033[48;5;" << bgColor << "m";
  }
  void setColor(const Color &fgColor, const Color &bgColor) override {
    std::cout << "\033[38;2;" << fgColor.r << ";" << fgColor.g << ";"
              << fgColor.b << "m\033[48;2;" << bgColor.r << ";" << bgColor.g << ";"
              << bgColor.b << "m";
  }
  void setDefaultColor() override {
    std::cout << "\033[39m\033[49m";
  }

  int getWidth() override {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (GetConsoleScreenBufferInfo(h, &m_info)) {
      return m_info.srWindow.Right - m_info.srWindow.Left + 1;
    }
    return 80;
  }
  int getHeight() override {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (GetConsoleScreenBufferInfo(h, &m_info)) {
      return m_info.srWindow.Bottom - m_info.srWindow.Top + 1;
    }
    return 24;
  }

  void moveUp(int amount) override {
    std::cout << "\033[" << amount << "A";
  }
  void moveDown(int amount) override {
    std::cout << "\033[" << amount << "B";
  }
  void moveRight(int amount) override {
    std::cout << "\033[" << amount << "C";
  }
  void moveLeft(int amount) override {
    std::cout << "\033[" << amount << "D";
  }
  void moveTo(int x, int y) override {
    std::cout << "\033[" << y << ";" << x << "H";
  }
  void moveToHead() override { moveTo(1, 1); }
  void put(int x, int y, char c) override {
    moveTo(x, y);
    std::cout << c;
  }
  void put(int x, int y, const char *s) override {
    moveTo(x, y);
    std::cout << s;
  }
  void clear() override {
    std::cout << "\033[H\033[2J";
  }

  // Call at end of frame
  void present() override { std::cout.flush(); }

  void write(const char* data, size_t size) override {
    if (m_hOut == INVALID_HANDLE_VALUE || size == 0) {
      std::cout.write(data, static_cast<std::streamsize>(size));
      std::cout.flush();
      return;
    }

    // Use native Windows API for bulk output (faster than iostream for large frames)
    DWORD written = 0;
    BOOL ok = WriteFile(m_hOut, data, static_cast<DWORD>(size), &written, nullptr);
    if (!ok) {
      // Fallback if WriteFile fails for some reason
      std::cout.write(data, static_cast<std::streamsize>(size));
      std::cout.flush();
    }
  }

private:
  bool m_enabled{false};
  DWORD m_prevMode{0};
  CONSOLE_SCREEN_BUFFER_INFO m_info;
  HANDLE m_hOut{INVALID_HANDLE_VALUE};
};
