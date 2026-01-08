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
 * @file test_assertion_movement.cpp
 * @brief Tests the correctness of the assertion movement diagnosis methods.
 */

#include "backend/diagnostics.h"
#include "common_fixtures.hpp"

#include <cstddef>
#include <gtest/gtest.h>
#include <set>
#include <utility>
#include <vector>

namespace mqt::debugger::test {

/**
 * @brief Fixture for testing the correctness of assertion movement on custom
 * code.
 *
 * This fixture sets up a DDSimulationState and provides the method
 * `loadCode` to load custom code into the state.
 */
class AssertionMovementTest : public CustomCodeFixture {
public:
  /**
   * @brief Checks a set of moved assertions against the expected ones.
   *
   * @param expected The assertions expected to be moved.
   */
  void checkMovements(const std::set<std::pair<size_t, size_t>>& expected) {
    std::vector<size_t> oldPositions(expected.size() + 1);
    std::vector<size_t> newPositions(expected.size() + 1);
    ASSERT_EQ(diagnostics->suggestAssertionMovements(
                  diagnostics, oldPositions.data(), newPositions.data(),
                  oldPositions.size()),
              expected.size());

    for (auto [oldPos, newPos] : expected) {
      oldPos += 2;
      newPos += 2;
      bool found = false;
      for (size_t i = 0; i < expected.size(); i++) {
        if (oldPositions[i] == oldPos) {
          ASSERT_EQ(newPositions[i], newPos)
              << "Expected " << (newPos - 2) << " but got "
              << (newPositions[i] - 2) << " for assertion at " << (oldPos - 2);
          found = true;
          break;
        }
      }
      ASSERT_TRUE(found) << "No suggestion found for assertion at "
                         << (oldPos - 2);
    }
  }
};

/**
 * @test Test the movement of an assertion over an instruction that
 * does not use any of the assertion's qubits.
 */
TEST_F(AssertionMovementTest, MoveOverIndependentInstructions) {
  loadCode(3, 3, R"(
  h q[0];
  cx q[0], q[1];
  cx q[0], q[2];
  x q[2];
  assert-eq 0.9, q[0], q[1] { 1, 0, 0, 0 }
  )");

  const std::set<std::pair<size_t, size_t>> expected = {{4, 3}};
  checkMovements(expected);
}

/**
 * @test Test the movement of an entanglement assertion over a single-qubit
 * gate.
 */
TEST_F(AssertionMovementTest, MoveEntOverSingleQubit) {
  loadCode(3, 3, R"(
  h q[0];
  cx q[0], q[1];
  h q[0];
  x q[0];
  h q[1];
  x q[1];
  assert-ent q[0], q[1];
  )");

  const std::set<std::pair<size_t, size_t>> expected = {{6, 2}};
  checkMovements(expected);
}

/**
 * @test Test the movement of a superposition assertion over (non-)diagonal
 * gates.
 */
TEST_F(AssertionMovementTest, MoveSupOverSpecificGates) {
  loadCode(3, 3, R"(
  h q[0];
  cx q[0], q[1];
  h q[0];
  y q[0];
  s q[0];
  h q[1];
  x q[1];
  assert-sup q[0];
  )");

  const std::set<std::pair<size_t, size_t>> expected = {{7, 3}};
  checkMovements(expected);
}

/**
 * @test Test the movement of an assertion over a barrier instruction.
 */
TEST_F(AssertionMovementTest, MoveOverBarrier) {
  loadCode(3, 3, R"(
  h q[0];
  barrier q[0];
  assert-sup q[0];
  )");

  const std::set<std::pair<size_t, size_t>> expected = {{2, 1}};
  checkMovements(expected);
}

/**
 * @test Test the movement of an assertion over a function definition.
 */
TEST_F(AssertionMovementTest, MoveThroughFunctionDefinition) {
  loadCode(3, 3, R"(
  h q[0];
  gate test q { h q; }
  assert-sup q[0];
  )");

  const std::set<std::pair<size_t, size_t>> expected({{4, 1}});
  checkMovements(expected);
}

/**
 * @test Test the movement of an assertion over a variable declaration
 * that does not use the assertion's qubits.
 */
TEST_F(AssertionMovementTest, MoveThroughOtherVariableDeclarations) {
  loadCode(3, 3, R"(
  h q[0];
  qreg x[2];
  creg y[2];
  assert-sup q[0];
  )");

  const std::set<std::pair<size_t, size_t>> expected({{3, 1}});
  checkMovements(expected);
}

/**
 * @test Test the assertion is not moved over a variable declaration
 * that uses the assertion's qubits.
 */
TEST_F(AssertionMovementTest, DontMoveThroughOwnVariableDeclarations) {
  loadCode(3, 3, R"(
  h q[0];
  qreg x[2];
  x x[0];
  assert-sup x[0];
  )");

  const std::set<std::pair<size_t, size_t>> expected({{3, 2}});
  checkMovements(expected);
}

/**
 * @test Test the assertion is not moved over a measurement.
 */
TEST_F(AssertionMovementTest, DontMoveThroughMeasurements) {
  loadCode(3, 3, R"(
  h q[0];
  measure q[1] -> c[1];
  assert-ent q[0];
  )");

  const std::set<std::pair<size_t, size_t>> expected;
  checkMovements(expected);
}

/**
 * @test Test the assertion is not moved over a reset.
 */
TEST_F(AssertionMovementTest, DontMoveThroughResets) {
  loadCode(3, 3, R"(
  h q[0];
  reset q[1];
  assert-ent q[0];
  )");

  const std::set<std::pair<size_t, size_t>> expected;
  checkMovements(expected);
}

/**
 * @test Test the entanglement assertion is moved even over a custom gate call.
 */
TEST_F(AssertionMovementTest, MoveThroughFunctionCalls) {
  loadCode(3, 3, R"(
  cx q[0], q[1];
  gate test q { h q; }
  test q[0];
  assert-ent q[0];
  )");

  const std::set<std::pair<size_t, size_t>> expected{{5, 1}};
  checkMovements(expected);
}

/**
 * @test Test the superposition assertion is moved over a custom gate call
 * that uses the same qubit.
 */
TEST_F(AssertionMovementTest, DontMoveThroughRelatedFunctionCall) {
  loadCode(3, 3, R"(
  h q[0];
  gate test q { h q; }
  test q[0];
  assert-sup q[0];
  )");

  const std::set<std::pair<size_t, size_t>> expected;
  checkMovements(expected);
}

/**
 * @test Test that an assertion is not moved over an instruction
 * that broadcasts to multiple qubits, including the assertion's qubit.
 */
TEST_F(AssertionMovementTest, DontMoveThroughBroadcastInstructions) {
  loadCode(3, 3, R"(
  h q[0];
  h q;
  assert-sup q[0];
  )");

  const std::set<std::pair<size_t, size_t>> expected;
  checkMovements(expected);
}

/**
 * @test Test that a broadcast-style assertion is not moved over an instruction
 * that uses one of the qubits in the broadcast.
 */
TEST_F(AssertionMovementTest, DontMoveBroadcastAssertion) {
  loadCode(3, 3, R"(
  h q[0];
  x q;
  x q[1];
  assert-sup q;
  )");

  const std::set<std::pair<size_t, size_t>> expected{{3, 1}};
  checkMovements(expected);
}

/**
 * @test Test that an assertion is not moved over a blocking instruction,
 * even if the instruction is in a classical control branch.
 */
TEST_F(AssertionMovementTest, RelatedClassicControlledGate) {
  loadCode(3, 3, R"(
  x q[0];
  measure q[0] -> c[0];
  if(c == 0) cx q[1], q[2];
  x q[1];
  assert-ent q[1], q[2];
  )");

  const std::set<std::pair<size_t, size_t>> expected({{4, 3}});
  checkMovements(expected);
}

/**
 * @test Test that an assertion is moved over an instruction that is in a
 * classical control branch, but does not use the assertion's qubits.
 */
TEST_F(AssertionMovementTest, UnrelatedClassicControlledGate) {
  loadCode(3, 3, R"(
  x q[0];
  measure q[0] -> c[0];
  if(c == 0) x q[2];
  x q[1];
  assert-ent q[1], q[2];
  )");

  const std::set<std::pair<size_t, size_t>> expected({{4, 2}});
  checkMovements(expected);
}

/**
 * @test Test that an assertion can be moved even inside custom gate calls.
 */
TEST_F(AssertionMovementTest, MoveInsideCustomGate) {
  loadCode(3, 3, R"(
  gate test q {
    h q;
    x q;
    assert-sup q;
  }
  )");

  const std::set<std::pair<size_t, size_t>> expected({{3, 2}});
  checkMovements(expected);
}

/**
 * @test Test that an assertion can be moved inside custom gate calls.
 * but is not moved out of it.
 */
TEST_F(AssertionMovementTest, DontMoveOutsideOfCustomGate) {
  loadCode(3, 3, R"(
  gate test t {
    x t;
    assert-sup t;
  }
  test q[0];
  )");

  const std::set<std::pair<size_t, size_t>> expected{{2, 1}};
  checkMovements(expected);
}

} // namespace mqt::debugger::test
