#pragma once

#include <algorithm>
#include <fcntl.h>
#include <iostream>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <vector>

#include "Terminal.hpp"

class TerminalLinux : public Terminal {
public:
  TerminalLinux() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    ::ioctl(STDOUT_FILENO, TIOCGWINSZ, &m_window);

    // Save original terminal state
    if (::tcgetattr(STDIN_FILENO, &m_origTermios) == 0) {
      m_rawMode = true;
    }

    std::cout << ALTBUF_ENTER << "\033[?25l";
    clear();
    std::cout.flush();
  }

  virtual ~TerminalLinux() {
    disableMouse();
    if (m_rawMode) {
      ::tcsetattr(STDIN_FILENO, TCSANOW, &m_origTermios);
    }
    std::cout << "\033[?25h" << ALTBUF_EXIT;
    std::cout.flush();
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
    ::ioctl(STDOUT_FILENO, TIOCGWINSZ, &m_window);
    return m_window.ws_col;
  }
  int getHeight() override {
    ::ioctl(STDOUT_FILENO, TIOCGWINSZ, &m_window);
    return m_window.ws_row;
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
    // Home + erase screen (not scrollback). For full reset use \033[3J if desired.
    std::cout << "\033[H\033[2J";
  }
  // Call at end of frame for smooth output
  void present() override { std::cout.flush(); }

  void write(const char* data, size_t size) override {
    std::cout.write(data, static_cast<std::streamsize>(size));
    std::cout.flush();
  }

  void enableMouse(bool trackMotion = true) override {
    if (m_mouseEnabled) return;

    // Put terminal into cbreak/raw-ish mode so we can read mouse reports immediately
    if (m_rawMode) {
      ::termios raw = m_origTermios;
      raw.c_lflag &= ~(ICANON | ECHO);
      raw.c_cc[VMIN] = 0;   // non-blocking
      raw.c_cc[VTIME] = 0;
      ::tcsetattr(STDIN_FILENO, TCSANOW, &raw);

      // Make stdin non-blocking at fd level too (helps pollMouse)
      int flags = ::fcntl(STDIN_FILENO, F_GETFL, 0);
      ::fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    }

    // Enable SGR extended mouse mode (best for coordinates > 223)
    // 1000 = basic click, 1002 = button+drag, 1003 = any motion
    // 1006 = SGR extended reporting
    std::cout << "\033[?1000h\033[?1002h";
    if (trackMotion) std::cout << "\033[?1003h";
    std::cout << "\033[?1006h";
    std::cout.flush();

    m_mouseEnabled = true;
    m_inputBuf.clear();
  }

  void disableMouse() override {
    if (!m_mouseEnabled) return;

    std::cout << "\033[?1000l\033[?1002l\033[?1003l\033[?1006l";
    std::cout.flush();

    if (m_rawMode) {
      ::tcsetattr(STDIN_FILENO, TCSANOW, &m_origTermios);
      int flags = ::fcntl(STDIN_FILENO, F_GETFL, 0);
      ::fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK);
    }

    m_mouseEnabled = false;
  }

  bool pollMouse(MouseState& out) override {
    if (!m_mouseEnabled) return false;

    // Read all available bytes (non-blocking)
    char buf[256];
    ssize_t n = ::read(STDIN_FILENO, buf, sizeof(buf));
    if (n > 0) {
      m_inputBuf.insert(m_inputBuf.end(), buf, buf + n);
    }

    // Try to parse SGR mouse sequences: \e[<button;x;y;M  or  \e[<button;x;y;m
    bool updated = false;

    while (m_inputBuf.size() >= 6) {  // minimum size for a mouse report
      // Find start of possible sequence
      auto it = std::find(m_inputBuf.begin(), m_inputBuf.end(), '\x1b');
      if (it == m_inputBuf.end()) {
        m_inputBuf.clear();
        break;
      }

      // Remove garbage before ESC
      if (it != m_inputBuf.begin()) {
        m_inputBuf.erase(m_inputBuf.begin(), it);
      }

      if (m_inputBuf.size() < 6) break;

      // Must be \e[<
      if (m_inputBuf[1] != '[' || m_inputBuf[2] != '<') {
        m_inputBuf.erase(m_inputBuf.begin());
        continue;
      }

      // Find 'M' or 'm'
      auto endIt = std::find(m_inputBuf.begin() + 3, m_inputBuf.end(), 'M');
      if (endIt == m_inputBuf.end()) {
        endIt = std::find(m_inputBuf.begin() + 3, m_inputBuf.end(), 'm');
      }
      if (endIt == m_inputBuf.end()) break; // incomplete

      // Parse button;x;y
      // Format after \e[< is:  button ; x ; y  then M/m
      std::string seq(m_inputBuf.begin() + 3, endIt + 1);
      int button = 0, x = 0, y = 0;
      char action = 0;

      if (sscanf(seq.c_str(), "%d;%d;%d%c", &button, &x, &y, &action) == 4) {
        // SGR: button 0 = left, 1 = middle, 2 = right. Bit 5 (32) = motion, bit 6 = release?
        bool isRelease = (action == 'm');
        int btn = button & 0b11;

        m_mouse.x = x - 1;  // 1-based → 0-based
        m_mouse.y = y - 1;

        if (!isRelease) {
          m_mouse.left   = (btn == 0);
          m_mouse.middle = (btn == 1);
          m_mouse.right  = (btn == 2);
        } else {
          // On release, clear the corresponding button
          if (btn == 0) m_mouse.left = false;
          if (btn == 1) m_mouse.middle = false;
          if (btn == 2) m_mouse.right = false;
        }

        m_mouse.anyButtonDown = m_mouse.left || m_mouse.middle || m_mouse.right;
        updated = true;
      }

      // Remove processed sequence
      m_inputBuf.erase(m_inputBuf.begin(), endIt + 1);
    }

    if (updated) {
      out = m_mouse;
    }
    return updated;
  }

private:
  ::winsize m_window;
  ::termios m_origTermios{};
  bool m_rawMode = false;

  // Mouse state
  MouseState m_mouse{};
  std::vector<char> m_inputBuf;   // for sequence accumulation
  bool m_mouseEnabled = false;
};
