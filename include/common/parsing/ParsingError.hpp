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
 * @file ParsingError.hpp
 * @brief Header file for the ParsingError class
 */

#pragma once

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>

namespace mqt::debugger {

/**
 * @brief Represents an error that occurred during parsing.
 */
struct ParsingErrorLocation {
  size_t line;
  size_t column;
  std::string detail;
};

class ParsingError : public std::runtime_error {
public:
  /**
   * @brief Constructs a new ParsingError with the given message.
   * @param msg The error message.
   */
  explicit ParsingError(const std::string& msg);

  /**
   * @brief Constructs a new ParsingError with a structured location.
   * @param line The 1-based line number.
   * @param column The 1-based column number.
   * @param detail The error detail text.
   */
  ParsingError(size_t line, size_t column, std::string detail);

  /**
   * @brief Returns the location information if available.
   * @return Pointer to the location info, or nullptr if absent.
   */
  const ParsingErrorLocation* location() const noexcept;

private:
  std::optional<ParsingErrorLocation> location_;
};

} // namespace mqt::debugger
