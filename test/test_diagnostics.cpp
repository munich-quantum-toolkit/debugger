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
 * @file test_diagnostics.cpp
 * @brief Test the functionality of the diagnostics module.
 */

#include "backend/debug.h"
#include "backend/diagnostics.h"
#include "common.h"
#include "common_fixtures.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <gtest/gtest.h>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace mqt::debugger::test {

/**
 * @brief Fixture for testing the correctness of the diagnostics module.
 *
 * This fixture creates a DDSimulationState and allows to load code from files
 * in the `circuits` directory.
 */
class DiagnosticsTest : public LoadFromFileFixture {};

/**
 * @test Test the correctness of the data dependencies retrieval.
 */
TEST_F(DiagnosticsTest, DataDependencies) {
  loadFromFile("failing-assertions");
  const std::map<size_t, std::set<size_t>> expected = {
      {1, {1}},
      {2, {1, 2}},
      {3, {1, 2, 3}},
      {4, {1, 2, 4}},
      {5, {1, 2, 3, 4, 5}},
      {6, {1, 2, 3, 4, 6}},
      {7, {1, 2, 4, 7}},
      {8, {1, 2, 4, 7, 8}},
      {9, {1, 2, 4, 9}},
      {10, {1, 2, 3, 4, 7, 8, 10}}};

  for (const auto& [instruction, expectedDependencies] : expected) {
    std::vector<uint8_t> dependencies(state->getInstructionCount(state), 0);
    // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
    diagnostics->getDataDependencies(
        diagnostics, instruction, false,
        reinterpret_cast<bool*>(dependencies.data()));
    // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
    std::set<size_t> dependenciesSet;
    for (size_t i = 0; i < dependencies.size(); ++i) {
      if (dependencies[i] != 0) {
        dependenciesSet.insert(i);
      }
    }
    ASSERT_EQ(dependenciesSet, expectedDependencies)
        << "Failed for instruction " << instruction;
  }
}

/**
 * @test Test whether error causes related to zero-controls are correctly
 * identified.
 */
TEST_F(DiagnosticsTest, ControlAlwaysZeroTest) {
  loadFromFile("failing-assertions");
  state->runSimulation(state);
  std::array<ErrorCause, 10> problems{};
  const size_t count =
      diagnostics->potentialErrorCauses(diagnostics, problems.data(), 10);
  ASSERT_EQ(count, 1);
  ASSERT_EQ(problems[0].type, ErrorCauseType::ControlAlwaysZero);
  ASSERT_EQ(problems[0].instruction, 4);
}

/**
 * @test Test whether error causes related to zero-controls are correctly
 * identified, even when the maximum number of problems is reached.
 */
TEST_F(DiagnosticsTest, MaximumControlAlwaysZeroTest) {
  loadFromFile("failing-assertions-multiple-zero-controls");
  state->runSimulation(state);
  std::array<ErrorCause, 10> problems{};
  ASSERT_EQ(diagnostics->potentialErrorCauses(diagnostics, problems.data(), 10),
            3);
  ASSERT_EQ(diagnostics->potentialErrorCauses(diagnostics, problems.data(), 3),
            3);
  ASSERT_EQ(diagnostics->potentialErrorCauses(diagnostics, problems.data(), 2),
            2);
}

/**
 * @test Test whether error causes related to missing interactions are correctly
 * identified.
 */
TEST_F(DiagnosticsTest, MissingInteraction) {
  loadFromFile("failing-assertions-missing-interaction");
  state->runSimulation(state);
  ASSERT_EQ(state->getCurrentInstruction(state), 7);
  std::array<ErrorCause, 10> problems{};
  const size_t count =
      diagnostics->potentialErrorCauses(diagnostics, problems.data(), 10);
  ASSERT_EQ(count, 1);
  ASSERT_EQ(problems[0].type, ErrorCauseType::MissingInteraction);
  ASSERT_EQ(problems[0].instruction, 7);
}

/**
 * @test Test whether error causes related to missing interactions are correctly
 * identified, even when the maximum number of problems is reached.
 */
TEST_F(DiagnosticsTest, MaximumMissingInteraction) {
  loadFromFile("failing-assertions-multiple-missing-interaction");
  state->runSimulation(state);
  ASSERT_EQ(state->getCurrentInstruction(state), 2);
  std::array<ErrorCause, 20> problems{};
  ASSERT_EQ(diagnostics->potentialErrorCauses(diagnostics, problems.data(), 20),
            10);
  ASSERT_EQ(diagnostics->potentialErrorCauses(diagnostics, problems.data(), 10),
            10);
  ASSERT_EQ(diagnostics->potentialErrorCauses(diagnostics, problems.data(), 3),
            3);
}

/**
 * @test Test whether mixed error causes are correctly identified, even when the
 * maximum number of problems is reached.
 */
TEST_F(DiagnosticsTest, MaximumMultipleCauses) {
  loadFromFile("failing-assertions-multiple-causes");
  state->runSimulation(state);
  ASSERT_EQ(state->getCurrentInstruction(state), 3);
  std::array<ErrorCause, 20> problems{};
  const std::vector<size_t> maxErrors = {20, 8, 7, 4};
  const std::vector<std::pair<size_t, std::vector<ErrorCauseType>>>
      expectedTypes = {
          {9,
           {MissingInteraction, MissingInteraction, MissingInteraction,
            MissingInteraction, MissingInteraction, MissingInteraction,
            MissingInteraction, ControlAlwaysZero, ControlAlwaysZero}},
          {8,
           {MissingInteraction, MissingInteraction, MissingInteraction,
            MissingInteraction, MissingInteraction, MissingInteraction,
            MissingInteraction, ControlAlwaysZero}},
          {7,
           {MissingInteraction, MissingInteraction, MissingInteraction,
            MissingInteraction, MissingInteraction, MissingInteraction,
            MissingInteraction}},
          {4,
           {MissingInteraction, MissingInteraction, MissingInteraction,
            MissingInteraction}},
      };
  for (size_t i = 0; i < maxErrors.size(); i++) {
    const auto [expectedCount, types] = expectedTypes[i];
    ASSERT_EQ(diagnostics->potentialErrorCauses(diagnostics, problems.data(),
                                                maxErrors[i]),
              expectedCount);
    for (size_t j = 0; j < expectedCount; j++) {
      ASSERT_EQ(problems.at(j).type, types[j]);
    }
  }
}

/**
 * @test Test that `potentialErrorCauses` works correctly even when there are no
 * failed assertions.
 */
TEST_F(DiagnosticsTest, NoFailedAssertions) {
  loadFromFile("complex-jumps");
  state->runSimulation(state);
  std::array<ErrorCause, 5> problems{};
  ASSERT_EQ(diagnostics->potentialErrorCauses(diagnostics, problems.data(), 5),
            0);
}

/**
 * @test Test that `potentialErrorCauses` works correctly even when the maximum
 * number of allowed problems is zero.
 */
TEST_F(DiagnosticsTest, RequestZeroProblems) {
  loadFromFile("failing-assertions");
  state->runSimulation(state);
  std::array<ErrorCause, 10> problems{};
  const size_t count =
      diagnostics->potentialErrorCauses(diagnostics, problems.data(), 0);
  ASSERT_EQ(count, 0);
}

/**
 * @test Test the correctness of the `getZeroControlInstructions` method of the
 * diagnostics interface.
 *
 * This is tested on code that includes jumps.
 */
TEST_F(DiagnosticsTest, ZeroControlsWithJumps) {
  loadFromFile("zero-controls-with-jumps");
  state->runSimulation(state);
  std::array<bool, 13> zeroControls{};
  diagnostics->getZeroControlInstructions(diagnostics, zeroControls.data());
  for (size_t i = 0; i < zeroControls.size(); i++) {
    ASSERT_FALSE(zeroControls.at(i) ^ (i == 3 || i == 12));
  }
}

/**
 * @test Test the correctness of the `getDataDependencies` method of the
 * diagnostics interface in the presence of jumps.
 */
TEST_F(DiagnosticsTest, DataDependenciesWithJumps) {
  loadFromFile("diagnose-with-jumps");
  const std::map<size_t, std::set<size_t>> expected = {
      {1, {1}},
      {2, {2, 13, 7, 5, 1}},
      {3, {3}},

      {5, {5}},
      {6, {6, 5}},
      {7, {7, 5}},
      {8, {8}},

      {10, {10}},
      {13, {13}},
      {11, {11}},
      {9, {9}},
      {14, {14}},
      {12, {12}},

      {15, {15}},

      {16, {16}},

      {17, {17, 16}},

      {18, {18, 13, 10, 7, 6, 5, 2, 1, 17, 16}}};

  for (const auto& pair : expected) {
    std::vector<uint8_t> dependencies(state->getInstructionCount(state), 0);
    // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
    diagnostics->getDataDependencies(
        diagnostics, pair.first, false,
        reinterpret_cast<bool*>(dependencies.data()));
    // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
    std::set<size_t> dependenciesSet;
    for (size_t i = 0; i < dependencies.size(); ++i) {
      if (dependencies[i] != 0) {
        dependenciesSet.insert(i);
      }
    }
    ASSERT_EQ(dependenciesSet, pair.second)
        << "Failed for instruction " << pair.first;
  }
}

/**
 * @test Test the correctness of the `getInteractions` method of the diagnostics
 * interface in the presence of jumps.
 */
TEST_F(DiagnosticsTest, InteractionsWithJumps) {
  loadFromFile("diagnose-with-jumps");

  const std::map<std::pair<size_t, size_t>, std::set<size_t>> expected = {
      {{1, 0}, {0}},        {{1, 1}, {1}},        {{1, 2}, {2}},
      {{2, 0}, {0, 1}},     {{2, 1}, {0, 1}},     {{2, 2}, {2}},

      {{5, 0}, {0}},        {{6, 0}, {1, 0}},     {{7, 1}, {0, 1}},

      {{10, 0}, {0}},

      {{17, 0}, {0}},       {{18, 0}, {1, 2, 0}}, {{18, 1}, {0, 2, 1}},
      {{18, 2}, {0, 1, 2}}, {{18, 3}, {3}}};

  for (const auto& pair : expected) {
    std::vector<uint8_t> interactions(state->getNumQubits(state), 0);
    // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
    diagnostics->getInteractions(diagnostics, pair.first.first,
                                 pair.first.second,
                                 reinterpret_cast<bool*>(interactions.data()));
    // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
    std::set<size_t> interactionsSet;
    for (size_t i = 0; i < interactions.size(); ++i) {
      if (interactions[i] != 0) {
        interactionsSet.insert(i);
      }
    }
    ASSERT_EQ(interactionsSet, pair.second)
        << "Failed for instruction " << pair.first.first << " qubit "
        << pair.first.second;
  }
}

/**
 * @test Test that at runtime, the interaction retrieval correctly identifies
 * existing interactions even outside of the current scope.
 */
TEST_F(DiagnosticsTest, RuntimeInteractions) {
  loadFromFile("runtime-interaction");
  ASSERT_EQ(state->runSimulation(state), OK);
  ASSERT_TRUE(state->didAssertionFail(state));
  ASSERT_EQ(state->getCurrentInstruction(state), 3);

  std::array<ErrorCause, 10> errors{};
  ASSERT_EQ(diagnostics->potentialErrorCauses(diagnostics, errors.data(), 10),
            1);

  // Interaction happens outside of the instruction, so we don't expect a
  // missing interaction.

  ASSERT_EQ(errors[0].type, ErrorCauseType::ControlAlwaysZero);
  ASSERT_EQ(errors[0].instruction, 6);
}

} // namespace mqt::debugger::test
