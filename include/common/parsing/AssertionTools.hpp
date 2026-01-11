/*
 * Copyright (c) 2024 - 2026 Chair for Design Automation, TUM
 * Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

#pragma once

#include "AssertionParsing.hpp"
#include "CodePreprocessing.hpp"

#include <cstdint>
#include <memory>

namespace mqt::debugger {

/**
 * @brief Define a general commutation rule.
 *
 * @param name The name of the commutation rule.
 * @param expression The expression that performs the check.
 */
#define COMMUTATION_RULE_GENERAL(name, expression)                             \
  COMMUTATION_RULE_TEMPLATE(Assertion, name, expression)

/**
 * @brief Define a commutation rule for entanglement assertions.
 *
 * @param name The name of the commutation rule.
 * @param expression The expression that performs the check.
 */
#define COMMUTATION_RULE_ENT(name, expression)                                 \
  COMMUTATION_RULE_TEMPLATE(EntanglementAssertion, name, expression)

/**
 * @brief Define a commutation rule for superposition assertions.
 *
 * @param name The name of the commutation rule.
 * @param expression The expression that performs the check.
 */
#define COMMUTATION_RULE_SUP(name, expression)                                 \
  COMMUTATION_RULE_TEMPLATE(SuperpositionAssertion, name, expression)

/**
 * @brief Define a template for commutation rules.
 *
 * @param type The type of assertion to check.
 * @param name The name of the commutation rule.
 * @param expression The expression that performs the check.
 */
#define COMMUTATION_RULE_TEMPLATE(type, name, expression)                      \
  const auto name = [](const type* assertion,                                  \
                       const std::string& instructionName,                     \
                       const std::vector<std::string>& arguments) {            \
    (void)assertion;                                                           \
    (void)instructionName;                                                     \
    (void)arguments;                                                           \
    return expression;                                                         \
  }

/**
 * @brief The possible results of a commutation check.
 *
 * Can be either `Commutes`, `DoesNotCommute`, or `Unknown`.
 */
enum class CommutationResult : uint8_t {
  /**
   * @brief Indicates that the instructions commute with certainty.
   */
  Commutes,
  /**
   * @brief Indicates that the instructions do not commute with certainty.
   */
  DoesNotCommute,
  /**
   * @brief Indicates that it cannot be said with certainty whether the
   * instructions commute or not.
   */
  Unknown,
};

/**
 * @brief Check if an assertion commutes with an instruction.
 * @param assertion The assertion to check.
 * @param instruction The instruction to check.
 * @return True if the assertion commutes with the instruction, false otherwise.
 */
bool doesCommute(const std::unique_ptr<Assertion>& assertion,
                 const Instruction& instruction);

} // namespace mqt::debugger
