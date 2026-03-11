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
#include <stdexcept>
#include <string>

namespace mqt::debugger {

/**
 * @brief Represents an error that occurred during parsing.
 */
class ParsingError : public std::runtime_error {
public:
  /**
   * @brief Constructs a new ParsingError with the given message.
   * @param msg The error message.
   */
  explicit ParsingError(const std::string& msg);

  /**
   * @brief Constructs a new ParsingError with location information.
   * @param line The one-based line number, or 0 if unknown.
   * @param column The one-based column number, or 0 if unknown.
   * @param detail The error detail message.
   */
  ParsingError(size_t line, size_t column, std::string detail);

  /**
   * @brief Constructs a new ParsingError with location information and message.
   * @param line The one-based line number, or 0 if unknown.
   * @param column The one-based column number, or 0 if unknown.
   * @param detail The error detail message.
   * @param message The formatted error message.
   */
  ParsingError(size_t line, size_t column, std::string detail,
               const std::string& message);

  /**
   * @brief Gets the line number of the error location, or 0 if unknown.
   */
  size_t line() const noexcept;

  /**
   * @brief Gets the column number of the error location, or 0 if unknown.
   */
  size_t column() const noexcept;

  /**
   * @brief Gets the error detail message.
   */
  const std::string& detail() const noexcept;

private:
  size_t line_ = 0;
  size_t column_ = 0;
  std::string detail_;
};

} // namespace mqt::debugger
