#pragma once

#include "Terminal.hpp"

#include <cmath>
#include <iostream>
#include <sys/ioctl.h>
#include <unistd.h>

class TerminalLinux : public Terminal {
public:
  TerminalLinux() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    ::ioctl(STDOUT_FILENO, TIOCGWINSZ, &m_window);
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

  int getWidth() { return m_window.ws_col; }
  int getHeight() { return m_window.ws_row; }

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
  ::winsize m_window;
};
