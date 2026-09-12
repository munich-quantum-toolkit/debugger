/*
 * Copyright (c) 2024 - 2026 Chair for Design Automation, TUM
 * Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

/**
 * @file Renderer.cpp
 * @brief Implementation of the CLI rendering primitives.
 */

#include "frontend/cli/Renderer.hpp"

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <numeric>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace mqt::debugger {

std::string margins(std::string_view content) {
  std::string s = " ";
  s.append(content);
  s += ' ';
  return s;
}

std::string leftAlign(std::string_view content, size_t width) {
  std::string s{content};
  if (content.size() < width) {
    s.append(width - content.size(), ' ');
  }
  return s;
}

std::string rightAlign(std::string_view content, size_t width) {
  const auto pad = content.size() < width ? width - content.size() : 0;
  std::string s(pad, ' ');
  s.append(content);
  return s;
}

std::string bold(std::string_view content) {
  std::string s{ANSI_BOLD};
  s.append(content);
  s.append(ANSI_NORMAL);
  return s;
}

std::string bgColor(std::string_view content, std::string_view bg) {
  std::string s{bg};
  s.append(content);
  s.append(ANSI_RESET);
  return s;
}

std::string fgColor(std::string_view content, std::string_view fg) {
  std::string s{fg};
  s.append(content);
  s.append(ANSI_RESET);
  return s;
}

size_t colMaxWidth(const std::vector<std::vector<std::string>>& rows,
                   size_t col) {
  return std::ranges::max(rows | std::views::transform([col](const auto& row) {
                            return row[col].size();
                          }));
}

std::string join(const std::vector<std::string>& parts, std::string_view sep) {
  if (parts.empty()) {
    return {};
  }
  return std::accumulate(std::next(parts.begin()), parts.end(),
                         std::string{parts.front()},
                         [sep](std::string acc, std::string_view p) {
                           acc.append(sep);
                           acc.append(p);
                           return acc;
                         });
}

void clearScreen() { std::cout << ANSI_CLEAR_SCREEN; }

void printTable(std::string_view header,
                const std::vector<std::vector<std::string>>& rows) {
  if (rows.empty()) {
    return;
  }

  // Column widths: max cell size across all rows per column.
  const auto nCols = rows[0].size();
  std::vector<size_t> widths(nCols);
  std::ranges::transform(
      std::views::iota(size_t{0}, nCols), widths.begin(),
      [&rows](size_t col) { return colMaxWidth(rows, col); });

  // Header row.
  std::cout << bgColor(fgColor(margins(bold(header)), ANSI_FG_BLACK),
                       ANSI_BG_TABLE_HEADER)
            << "\n";

  // Data rows: alternate light/dark background, bold on even rows.
  for (size_t r = 0; r < rows.size(); ++r) {
    const bool even = (r % 2 == 0);
    std::vector<std::string> cells(nCols);
    std::ranges::transform(rows[r], widths, cells.begin(),
                           [even](std::string_view content, size_t width) {
                             const auto cell =
                                 margins(leftAlign(content, width));
                             return even ? bold(cell) : cell;
                           });
    const auto rowBg = even ? ANSI_BG_TABLE_ROW_EVEN : ANSI_BG_TABLE_ROW_ODD;
    const auto rowFg = even ? ANSI_FG_BLACK : ANSI_FG_WHITE;
    std::cout << bgColor(fgColor(join(cells, "|"), rowFg), rowBg) << "\n";
  }
}

} // namespace mqt::debugger
