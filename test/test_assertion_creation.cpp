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
 * @file test_assertion_creation.cpp
 * @brief Tests the correctness of the assertion creation diagnosis methods.
 */

#include "backend/debug.h"
#include "backend/diagnostics.h"
#include "common.h"
#include "common_fixtures.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <gtest/gtest.h>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace mqt::debugger::test {

/**
 * @brief Fixture for testing the correctness of assertion creation on custom
 * code.
 *
 * This fixture sets up a DDSimulationState and provides the method
 * `loadCode` to load custom code into the state.
 */
class AssertionCreationTest : public CustomCodeFixture {
public:
  /**
   * @brief Checks a set of created assertions against the expected ones.
   *
   * @param expected The assertions expected to be created.
   * @param expectedErrors The expected number of errors.
   */
  void
  checkNewAssertions(const std::set<std::pair<size_t, std::string>>& expected,
                     size_t expectedErrors) {
    size_t errors = 0;
    ASSERT_EQ(state->runAll(state, &errors), OK);
    ASSERT_EQ(errors, expectedErrors);
    std::vector<size_t> newPositions(expected.size() + 1);
    std::vector<std::array<char, 256>> newAssertions(expected.size() + 1);
    std::vector<char*> newAssertionsPointers(expected.size() + 1);
    std::ranges::transform(
        newAssertions, newAssertionsPointers.begin(),
        [](std::array<char, 256>& arr) { return arr.data(); });
    ASSERT_EQ(diagnostics->suggestNewAssertions(
                  diagnostics, newPositions.data(),
                  newAssertionsPointers.data(), newPositions.size()),
              expected.size());

    for (auto [pos, string] : expected) {
      pos += 2;
      bool found = false;
      for (size_t i = 0; i < expected.size(); i++) {
        if (newPositions[i] == pos && newAssertions[i].data() == string) {
          found = true;
          break;
        }
      }
      ASSERT_TRUE(found) << "No new assertion found for " << (pos - 2);
    }
  }
};

/**
 * @test Tests the creation of new entanglement assertions from bigger ones.
 *
 * The test starts with an assertion over `q[0]`, `q[1]`, and `q[2]` and should
 * end with three new assertions: `q[0], q[1]`; `q[0], q[2]`; and `q[1], q[2]`.
 */
TEST_F(AssertionCreationTest, CreateEntanglementAssertionFromBigAssertion) {
  loadCode(3, 3, R"(
  h q[0];
  cx q[0], q[1];
  cx q[2], q[1];
  assert-ent q[0], q[1], q[2];
  )");

  const std::set<std::pair<size_t, std::string>> expected = {
      {3, "assert-ent q[1], q[2];\n"},
      {3, "assert-ent q[0], q[2];\n"},
      {3, "assert-ent q[0], q[1];\n"}};
  checkNewAssertions(expected, 1);
}

/**
 * @test Tests the creation of new entanglement assertions from
 * a simple interaction graph.
 *
 * The test starts with an assertion over `q[0]` and `q[2]` and should generate
 * two new assertions: `q[0], q[1]`; and `q[1], q[2]`.
 */
TEST_F(AssertionCreationTest, CreateEntanglementAssertionFromTreeSimple) {
  loadCode(3, 3, R"(
  h q[0];
  cx q[0], q[1];
  cx q[2], q[1];
  assert-ent q[0], q[2];
  )");

  const std::set<std::pair<size_t, std::string>> expected = {
      {2, "assert-ent q[0], q[1];\n"}, {3, "assert-ent q[1], q[2];\n"}};
  checkNewAssertions(expected, 1);
}

/**
 * @test Tests the creation of new equality assertions by splitting a bigger
 * one.
 *
 * The test starts with an assertion over `q[0]` and `q[1]` and should generate
 * one new assertions for each qubit.
 */
TEST_F(AssertionCreationTest, SplitEqualityAssertion) {
  loadCode(2, 1, R"(
  x q[0];
  assert-eq q[0], q[1] { 1, 0, 0, 0 }
  )");

  const std::set<std::pair<size_t, std::string>> expected = {
      {1, "assert-eq q[0] { 1, 0 }\n"}, {1, "assert-eq q[1] { 1, 0 }\n"}};
  checkNewAssertions(expected, 1);
}

/**
 * @test Tests the creation of new equality assertions by splitting a bigger
 * one.
 *
 * The test starts with an assertion equally distributed over `q[0]` and `q[1]`
 * and should generate one new assertions for each qubit.
 */
TEST_F(AssertionCreationTest, SplitEqualityAssertionMultipleAmplitudes) {
  loadCode(3, 1, R"(
  assert-eq q[0], q[1] { 0.5, 0.5, 0.5, 0.5 }
  )");

  const std::set<std::pair<size_t, std::string>> expected = {
      {0, "assert-eq 0.99999, q[0] { 0.707107, 0.707107 }\n"},
      {0, "assert-eq 0.99999, q[1] { 0.707107, 0.707107 }\n"}};
  checkNewAssertions(expected, 1);
}

/**
 * @test Tests that the assertion creation does not split entangled equality
 * assertions.
 */
TEST_F(AssertionCreationTest, DontSplitEntangledEqualityAssertion) {
  loadCode(3, 1, R"(
  assert-eq 0.9, q[0], q[1] { 0.707, 0, 0, 0.707 }
  )");

  const std::set<std::pair<size_t, std::string>> expected = {};
  checkNewAssertions(expected, 1);
}

/**
 * @test Tests the creation of new equality assertions with rounding.
 */
TEST_F(AssertionCreationTest, SplitEqualityAssertionRounded) {
  loadCode(3, 1, R"(
  assert-eq 0.99999, q[0], q[1], q[2] { 0, 0, 0, 0.70711, 0, 0, -0, -0.70711 }
  )");

  const std::set<std::pair<size_t, std::string>> expected = {
      {0, "assert-eq 0.99999, q[0] { 0, 1 }\n"},
      {0, "assert-eq 0.99999, q[1] { 0, 1 }\n"},
      {0, "assert-eq 0.99999, q[2] { 0.707107, -0.707107 }\n"}};
  checkNewAssertions(expected, 1);
}

} // namespace mqt::debugger::test
