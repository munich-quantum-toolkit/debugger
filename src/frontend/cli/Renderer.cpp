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
 * @brief Implementation of the CLI rendering primitives and the `Renderer`
 * output class.
 */

#include "frontend/cli/Renderer.hpp"

#include <cstddef>
#include <iterator>
#include <numeric>
#include <ostream>
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
  std::string s{ansi::BOLD};
  s.append(content);
  s.append(ansi::NORMAL);
  return s;
}

std::string bgColor(std::string_view content, std::string_view bg) {
  std::string s{bg};
  s.append(content);
  s.append(ansi::RESET);
  return s;
}

std::string fgColor(std::string_view content, std::string_view fg) {
  std::string s{fg};
  s.append(content);
  s.append(ansi::RESET);
  return s;
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

Renderer::Renderer(std::ostream& outStream) : out(outStream) {}

void Renderer::clearScreen() { out << ansi::CLEAR_SCREEN; }

void Renderer::print(std::string_view s) { out << s; }

void Renderer::println(std::string_view s) { out << s << '\n'; }

} // namespace mqt::debugger
