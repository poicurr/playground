#pragma once

#include "Terminal.hpp"

#include <algorithm>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

// Simple cell for terminal canvas.
// For full unicode later we could use char32_t + width, but char + manual for now.
struct Cell {
  char ch = ' ';
  Terminal::Color fg{200, 200, 200};
  Terminal::Color bg{0, 0, 0};
};

// Canvas provides a simple 2D drawing surface with efficient present.
// Optimized for playground animations: one big write per frame with minimal ANSI.
class Canvas {
public:
  explicit Canvas(Terminal* term = nullptr)
      : m_term(term) {
    if (!m_term) {
#ifdef _WIN32
      // caller must pass one; we don't own
#else
#endif
    }
    resizeToTerminal();
  }

  void attach(Terminal* term) { m_term = term; resizeToTerminal(); }

  Terminal* terminal() const { return m_term; }

  int width() const { return m_w; }
  int height() const { return m_h; }

  // Query fresh size from terminal and reallocate buffers if changed.
  void resizeToTerminal() {
    if (!m_term) return;
    int nw = m_term->getWidth();
    int nh = m_term->getHeight();
    if (nw < 1) nw = 80;
    if (nh < 1) nh = 24;
    if (nw == m_w && nh == m_h) return;

    m_w = nw;
    m_h = nh;
    m_cells.assign(size_t(m_w) * m_h, Cell{});
  }

  void clear(const Cell& fill = Cell{' ', {180,180,180}, {}} ) {
    for (auto& c : m_cells) c = fill;
  }

  // Safe put (clamped)
  void put(int x, int y, char ch, const Terminal::Color& fg, const Terminal::Color& bg) {
    if (x < 0 || y < 0 || x >= m_w || y >= m_h) return;
    Cell& c = m_cells[y * m_w + x];
    c.ch = ch;
    c.fg = fg;
    c.bg = bg;
  }

  void put(int x, int y, const Cell& cell) {
    if (x < 0 || y < 0 || x >= m_w || y >= m_h) return;
    m_cells[y * m_w + x] = cell;
  }

  void put(int x, int y, char ch) {
    put(x, y, ch, {220,220,220}, {});
  }

  // Write text (stops at edge, no wrap)
  void text(int x, int y, std::string_view s, const Terminal::Color& fg, const Terminal::Color& bg) {
    for (char c : s) {
      if (x >= m_w) break;
      put(x++, y, c, fg, bg);
    }
  }

  // Fill a rectangular region [x0,x1) x [y0,y1)
  void fill(int x0, int y0, int x1, int y1, const Cell& c) {
    if (!m_term) return;
    x0 = std::max(0, x0); x1 = std::min(m_w, x1);
    y0 = std::max(0, y0); y1 = std::min(m_h, y1);
    for (int yy = y0; yy < y1; ++yy) {
      for (int xx = x0; xx < x1; ++xx) {
        m_cells[yy * m_w + xx] = c;
      }
    }
  }

  // Dim all cells' background by factor (0..1) for cheap trails/fades.
  // Also optionally dims fg.
  void dim(float factor = 0.85f, bool dimFg = false) {
    uint8_t f = static_cast<uint8_t>(std::clamp(factor, 0.0f, 1.0f) * 255.0f);
    for (auto& c : m_cells) {
      c.bg.r = static_cast<uint8_t>((int(c.bg.r) * f) >> 8);
      c.bg.g = static_cast<uint8_t>((int(c.bg.g) * f) >> 8);
      c.bg.b = static_cast<uint8_t>((int(c.bg.b) * f) >> 8);
      if (dimFg) {
        c.fg.r = static_cast<uint8_t>((int(c.fg.r) * f) >> 8);
        c.fg.g = static_cast<uint8_t>((int(c.fg.g) * f) >> 8);
        c.fg.b = static_cast<uint8_t>((int(c.fg.b) * f) >> 8);
      }
    }
  }

  // Direct cell access for advanced playground hacks (fire, sand, etc.)
  Cell& cell(int x, int y) { return m_cells[y * m_w + x]; }
  const Cell& cell(int x, int y) const { return m_cells[y * m_w + x]; }

  // Draw the current buffer to terminal using one optimized write.
  // Tracks last fg/bg/position to emit fewer sequences.
  void present() {
    if (!m_term || m_w <= 0 || m_h <= 0) return;

    // Build one big output string
    std::string out;
    out.reserve(size_t(m_w) * m_h * 3 + 64);

    Terminal::Color curFg{-1,-1,-1};
    Terminal::Color curBg{-1,-1,-1};
    int curX = -1, curY = -1;

    auto emitColor = [&](const Terminal::Color& fg, const Terminal::Color& bg) {
      if (fg.r != curFg.r || fg.g != curFg.g || fg.b != curFg.b ||
          bg.r != curBg.r || bg.g != curBg.g || bg.b != curBg.b) {
        out += "\033[38;2;";
        out += std::to_string(fg.r); out += ';';
        out += std::to_string(fg.g); out += ';';
        out += std::to_string(fg.b); out += "m";

        out += "\033[48;2;";
        out += std::to_string(bg.r); out += ';';
        out += std::to_string(bg.g); out += ';';
        out += std::to_string(bg.b); out += "m";
        curFg = fg;
        curBg = bg;
      }
    };

    auto moveTo = [&](int x, int y) {
      // 1-based for ANSI
      out += "\033[";
      out += std::to_string(y + 1);
      out += ';';
      out += std::to_string(x + 1);
      out += 'H';
      curX = x; curY = y;
    };

    // Start at home-ish, default colors will be set on first cell
    out += "\033[H"; // home (1,1)
    curX = 0; curY = 0;

    for (int y = 0; y < m_h; ++y) {
      for (int x = 0; x < m_w; ++x) {
        const Cell& cell = m_cells[y * m_w + x];

        // Move only if not sequential
        if (x != curX || y != curY) {
          moveTo(x, y);
        }

        emitColor(cell.fg, cell.bg);
        out += cell.ch;

        curX = x + 1;
        curY = y;
        if (curX >= m_w) {
          curX = 0;
          curY = y + 1;
        }
      }
    }

    // reset colors at end? optional. many apps leave it.
    // out += "\033[0m";

    // Single write - goes through Terminal's write() which can use native APIs on Windows
    if (m_term) {
      m_term->write(out.data(), out.size());
    } else {
      std::cout.write(out.data(), static_cast<std::streamsize>(out.size()));
      std::cout.flush();
    }
  }

private:
  Terminal* m_term = nullptr;
  std::vector<Cell> m_cells;
  int m_w = 0;
  int m_h = 0;
};
