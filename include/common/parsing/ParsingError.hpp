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
};

} // namespace mqt::debugger
