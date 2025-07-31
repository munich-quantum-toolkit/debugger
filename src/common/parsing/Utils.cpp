/*
 * Copyright (c) 2024 - 2025 Chair for Design Automation, TUM
 * Copyright (c) 2025 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

/**
 * @file Utils.cpp
 * @brief Implementation of utility functions used by the debugger.
 */

#include "common/parsing/Utils.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <ranges>
#include <string>
#include <vector>

namespace mqt::debugger {

std::string trim(const std::string& str) {
  auto start = std::ranges::find_if_not(str, ::isspace);
  auto end = std::ranges::find_if_not(std::ranges::reverse_view(str), ::isspace)
                 .base();
  return (start < end) ? std::string(start, end) : std::string();
}

std::vector<std::string> splitString(const std::string& text, char delimiter,
                                     bool includeEmpty) {
  const std::vector<char> delimiters{delimiter};
  return splitString(text, delimiters, includeEmpty);
}

std::vector<std::string> splitString(const std::string& text,
                                     const std::vector<char>& delimiters,
                                     bool includeEmpty) {
  std::vector<std::string> result;
  size_t pos = 0;
  while (true) {
    size_t min = std::string ::npos;
    for (const auto del : delimiters) {
      const size_t newPos = text.find(del, pos);
      min = newPos < min ? newPos : min;
    }
    if (min == std::string::npos) {
      break;
    }
    if (min > pos || includeEmpty) {
      result.push_back(text.substr(pos, min - pos));
    }
    pos = min + 1;
  }
  if (text.length() > pos || includeEmpty) {
    result.push_back(text.substr(pos, text.length() - pos));
  }
  return result;
}

std::string replaceString(std::string str, const std::string& from,
                          const std::string& to) {
  size_t startPos = 0;
  while ((startPos = str.find(from, startPos)) != std::string::npos) {
    str.replace(startPos, from.length(), to);
    startPos += to.length(); // Handles case where 'to' is a substring of 'from'
  }
  return str;
}

std::string removeWhitespace(std::string str) {
  std::erase_if(str, ::isspace);
  return str;
}

bool variablesEqual(const std::string& v1, const std::string& v2) {
  if (v1.find('[') != std::string::npos && v2.find('[') != std::string::npos) {
    return v1 == v2;
  }
  if (v1.find('[') != std::string::npos) {
    return variablesEqual(splitString(v1, '[')[0], v2);
  }
  if (v2.find('[') != std::string::npos) {
    return variablesEqual(splitString(v2, '[')[0], v1);
  }
  return v1 == v2;
}

std::string variableBaseName(const std::string& v) {
  return splitString(v, '[')[0];
}

} // namespace mqt::debugger
