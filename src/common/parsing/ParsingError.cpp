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

ParsingError::ParsingError(const std::string& msg) : std::runtime_error(msg) {}

} // namespace mqt::debugger
