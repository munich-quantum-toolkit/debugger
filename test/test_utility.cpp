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
 * @file test_utility.cpp
 * @brief Test the functionality of utility functions provided by the debugger.
 */
#include "backend/debug.h"
#include "common.h"
#include "common_fixtures.hpp"

#include <cstddef>
#include <gtest/gtest.h>
#include <map>
#include <string>
#include <utility>

namespace mqt::debugger::test {

/**
 * @brief Fixture for testing the correctness of utility functions.
 *
 * This fixture creates a DDSimulationState and allows to load code from files
 * in the `circuits` directory.
 */
class UtilityTest : public LoadFromFileFixture {};

/**
 * @test Test the retrieval of the number of instructions in the loaded code.
 */
TEST_F(UtilityTest, GetInstructionCount) {
  loadFromFile("complex-jumps");
  ASSERT_EQ(state->getInstructionCount(state), 15);
}

/**
 * @test Test the retrieval of the position of an instruction in the loaded
 * code.
 */
TEST_F(UtilityTest, GetInstructionPosition) {
  loadFromFile("complex-jumps");

  const std::map<size_t, std::pair<size_t, size_t>> expected = {
      {0, {0, 9}},     {1, {38, 112}},  {2, {79, 88}},
      {3, {112, 112}}, {4, {150, 298}}, {12, {452, 477}}};
  for (const auto& [instruction, expectedPosition] : expected) {
    size_t start = 0;
    size_t end = 0;
    state->getInstructionPosition(state, instruction, &start, &end);
    ASSERT_EQ(start, expectedPosition.first)
        << "Failed for instruction " << instruction;
    ASSERT_EQ(end, expectedPosition.second)
        << "Failed for instruction " << instruction;
  }
}

/**
 * @test Test that an error is returned when trying to access the position of an
 * instruction with index larger than the total number of instructions.
 */
TEST_F(UtilityTest, BadInstructionPosition) {
  loadFromFile("complex-jumps");

  size_t start = 0;
  size_t end = 0;
  ASSERT_EQ(state->getInstructionPosition(state, 100, &start, &end), ERROR);
}

} // namespace mqt::debugger::test
