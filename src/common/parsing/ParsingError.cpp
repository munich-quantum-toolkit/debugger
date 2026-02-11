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

#include <stdexcept>
#include <string>

namespace mqt::debugger {

ParsingError::ParsingError(const std::string& msg)
    : std::runtime_error(msg), detail_(msg) {}

ParsingError::ParsingError(size_t line, size_t column, std::string detail)
    : std::runtime_error(detail), line_(line), column_(column),
      detail_(std::move(detail)) {}

ParsingError::ParsingError(size_t line, size_t column, std::string detail,
                           const std::string& message)
    : std::runtime_error(message), line_(line), column_(column),
      detail_(std::move(detail)) {}

size_t ParsingError::line() const noexcept { return line_; }

size_t ParsingError::column() const noexcept { return column_; }

const std::string& ParsingError::detail() const noexcept { return detail_; }

} // namespace mqt::debugger
