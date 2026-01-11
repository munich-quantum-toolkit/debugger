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
 * @file ParsingError.cpp
 * @brief Implementation of the ParsingError class.
 */

#include "common/parsing/ParsingError.hpp"

#include <sstream>
#include <stdexcept>
#include <string>

namespace mqt::debugger {

ParsingError::ParsingError(const std::string& msg) : std::runtime_error(msg) {}

ParsingError::ParsingError(size_t line, size_t column, std::string detail)
    : std::runtime_error([line, column, &detail]() {
        std::ostringstream message;
        message << "<input>:" << line << ":" << column << ": " << detail;
        return message.str();
      }()),
      location_(ParsingErrorLocation{line, column, std::move(detail)}) {}

const ParsingErrorLocation* ParsingError::location() const noexcept {
  if (!location_.has_value()) {
    return nullptr;
  }
  return &location_.value();
}

} // namespace mqt::debugger
