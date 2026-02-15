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
 * @file test_custom_code.cpp
 * @brief Tests the correctness of the framework for smaller, very specific edge
 * cases that can be provided as custom code directly.
 *
 * These tests are typically used to verify bug fixes or cover edge cases that
 * are not covered by the other tests.
 */

#include "backend/debug.h"
#include "backend/diagnostics.h"
#include "common.h"
#include "common_fixtures.hpp"
#include "utils_test.hpp"

#include <array>
#include <cstddef>
#include <gtest/gtest.h>

namespace mqt::debugger::test {

/**
 * @brief Fixture for testing the correctness of the debugger on custom code.
 *
 * This fixture sets up a DDSimulationState and provides the method
 * `loadCode` to load custom code into the state.
 */
class CustomCodeTest : public CustomCodeFixture {};

/**
 * @test Test the usage of classically controlled operations where the condition
 * evaluates to false.
 */
TEST_F(CustomCodeTest, IfElseOperationFalse) {
  loadCode(2, 1,
           "z q[0];"
           "cx q[0], q[1];"
           "measure q[0] -> c[0];"
           "if(c==1) x q[1];"
           "if(c==0) z q[1];");
  ASSERT_EQ(state->runSimulation(state), OK);

  std::array<Complex, 4> amplitudes{};
  Statevector sv{2, 4, amplitudes.data()};
  state->getStateVectorFull(state, &sv);
  ASSERT_TRUE(complexEquality(amplitudes[0], 1, 0.0));

  ASSERT_EQ(state->stepBackward(state), OK);
}

/**
 * @test Test the usage of classically controlled operations where the condition
 * evaluates to true.
 */
TEST_F(CustomCodeTest, IfElseOperationTrue) {
  loadCode(2, 1,
           "x q[0];"
           "cx q[0], q[1];"
           "measure q[0] -> c[0];"
           "if(c==1) x q[1];"
           "if(c==0) z q[1];");
  ASSERT_EQ(state->runSimulation(state), OK);

  std::array<Complex, 4> amplitudes{};
  Statevector sv{2, 4, amplitudes.data()};
  state->getStateVectorFull(state, &sv);
  ASSERT_TRUE(complexEquality(amplitudes[1], 1, 0.0));

  ASSERT_EQ(state->stepBackward(state), OK);
}

/**
 * @test Test the usage of if-else operations with multiple gates.
 */
TEST_F(CustomCodeTest, IfElseOperationMulti) {
  loadCode(2, 1,
           "x q[0];"
           "measure q[0] -> c[0];"
           "if(c==1) { x q[0]; x q[1]; }");
  ASSERT_EQ(state->runSimulation(state), OK);

  std::array<Complex, 4> amplitudes{};
  Statevector sv{2, 4, amplitudes.data()};
  state->getStateVectorFull(state, &sv);
  ASSERT_TRUE(complexEquality(amplitudes[2], 1, 0.0));
}

/**
 * @test Test the `reset` instruction.
 */
TEST_F(CustomCodeTest, ResetGate) {
  loadCode(1, 1,
           "x q[0];"
           "z q[0];"
           "reset q[0];"
           "barrier;");
  Complex result;

  forwardTo(2);
  ASSERT_EQ(state->getAmplitudeIndex(state, 1, &result), OK);
  ASSERT_TRUE(complexEquality(result, -1.0, 0.0));

  forwardTo(3);
  ASSERT_EQ(state->getAmplitudeIndex(state, 0, &result), OK);
  ASSERT_TRUE(complexEquality(result, -1.0, 0.0));
}

/**
 * @test Test that parsing works correctly even if a custom gate name includes
 * the keyword `gate`.
 */
TEST_F(CustomCodeTest, GateInGateName) {
  loadCode(1, 1,
           "gate my_gate q0 {"
           "  x q0;"
           "}"
           "my_gate q[0];"
           "measure q[0] -> c[0];"
           "assert-eq q[0] { 0, 1 }");
  state->runSimulation(state);
}

/**
 * @test Test the different types of equality assertions.
 */
TEST_F(CustomCodeTest, EqualityAssertion) {
  loadCode(2, 0,
           "h q[0];"
           "cx q[0], q[1];"
           "assert-eq 0.9, q[0], q[1] { 0.707, 0, 0, 0.707 }"
           "assert-eq q[0], q[1] { qreg q[2]; h q[1]; cx q[1], q[0]; }");
  size_t numErrors = 0;
  ASSERT_EQ(state->runAll(state, &numErrors), OK);
  ASSERT_EQ(numErrors, 0);
}

/**
 * @test Test that destructive interference does not influence sub-state
 * vectors.
 */
TEST_F(CustomCodeTest, DestructiveInterference) {
  loadCode(3, 0,
           "x q[0];"
           "h q[0];"
           "h q[1];"
           "cx q[1], q[2];"
           "assert-sup q[1], q[2];"
           "assert-ent q[1], q[2];");
  size_t numErrors = 0;
  ASSERT_EQ(state->runAll(state, &numErrors), OK);
  ASSERT_EQ(numErrors, 0);
}

/**
 * @test Test that an error is returned if a sub-state vector is created where
 * included qubits are entangled with non-included qubits for statevector
 * equality assertions.
 */
TEST_F(CustomCodeTest, IllegalSubstateSVEqualityAssertion) {
  loadCode(3, 0,
           "x q[0];"
           "h q[0];"
           "h q[1];"
           "cx q[1], q[2];"
           "assert-eq 0.9, q[0], q[1] { 0.5, 0.5, 0.5, 0.5 }");
  size_t numErrors = 0;
  ASSERT_EQ(state->runAll(state, &numErrors), ERROR);
}

/**
 * @test Test that no error is returned if a sub-state vector is created where
 * included qubits are not entangled with non-included qubits for statevector
 * equality assertions.
 */
TEST_F(CustomCodeTest, LegalSubstateSVEqualityAssertion) {
  loadCode(3, 0,
           "x q[0];"
           "h q[0];"
           "h q[1];"
           "cx q[1], q[2];"
           "assert-eq 0.9, q[1], q[2] { 0.707, 0, 0, 0.707 }");
  size_t numErrors = 0;
  ASSERT_EQ(state->runAll(state, &numErrors), OK);
  ASSERT_EQ(numErrors, 0);
}

/**
 * @test Test that an error is returned if a sub-state vector is created where
 * included qubits are entangled with non-included qubits for circuit equality
 * assertions.
 */
TEST_F(CustomCodeTest, IllegalSubstateCircuitEqualityAssertion) {
  loadCode(3, 0,
           "x q[0];"
           "h q[0];"
           "h q[1];"
           "cx q[1], q[2];"
           "assert-eq 0.9, q[0], q[1] { qreg q[2]; h q[0]; h q[1]; }");
  size_t numErrors = 0;
  ASSERT_EQ(state->runAll(state, &numErrors), ERROR);
}

/**
 * @test Test that no error is returned if a sub-state vector is created where
 * included qubits are not entangled with non-included qubits for circuit
 * equality assertions.
 */
TEST_F(CustomCodeTest, LegalSubstateCircuitEqualityAssertion) {
  loadCode(3, 0,
           "x q[0];"
           "h q[0];"
           "h q[1];"
           "cx q[1], q[2];"
           "assert-eq 0.9, q[2], q[1] { qreg q[2]; h q[0]; cx q[0], q[1]; }");
  size_t numErrors = 0;
  ASSERT_EQ(state->runAll(state, &numErrors), OK);
  ASSERT_EQ(numErrors, 0);
}

/**
 * @test Test that an error in the code is correctly detected at "compile"-time.
 *
 * The error is caused by using an incorrect register name.
 */
TEST_F(CustomCodeTest, ErrorInCode) {
  loadCode(3, 0, "x f[0];", true);
  size_t numErrors = 0;
  ASSERT_EQ(state->runAll(state, &numErrors), ERROR);
  ASSERT_EQ(numErrors, 0);
  ASSERT_EQ(state->runSimulation(state), ERROR);
  ASSERT_EQ(state->runSimulationBackward(state), ERROR);
}

/**
 * @test Test that the stack trace cannot be retrieved if an error occurred when
 * loading the code.
 */
TEST_F(CustomCodeTest, StackTraceErrorInCode) {
  loadCode(3, 0, "x f[0];", true);
  size_t depth = 0;
  ASSERT_EQ(state->getStackDepth(state, &depth), ERROR);
  ASSERT_EQ(state->getStackTrace(state, 10, nullptr), ERROR);
}

/**
 * @test Test that an error is returned if a circuit equality assertion is
 * executed that tries to use an assertion in its body.
 */
TEST_F(CustomCodeTest, ErrorAssertionInCircuitEqualityAssertion) {
  loadCode(3, 0,
           "x q[0];"
           "assert-eq q[0], q[1], q[2] { qreg q[3]; assert-sup q[0]; }");
  ASSERT_EQ(state->runAll(state, nullptr), ERROR);
}

/**
 * @test Test the correctness of `barrier` instructions.
 */
TEST_F(CustomCodeTest, BarrierInstruction) {
  loadCode(1, 0,
           "barrier;"
           "x q[0];");
  ASSERT_EQ(state->stepForward(state), OK);
  ASSERT_EQ(state->stepForward(state), OK);

  ASSERT_EQ(state->stepForward(state), OK);
  ASSERT_EQ(state->stepBackward(state), OK);
  ASSERT_EQ(state->stepOverForward(state), OK);
  ASSERT_EQ(state->stepOverBackward(state), OK);
}

/**
 * @test Test that an error is returned at parse-time if an invalid register
 * index is accessed by an assertion.
 */
TEST_F(CustomCodeTest, ErrorAssertionInvalidIndex) {
  loadCode(3, 0,
           "x q[0];"
           "assert-sup q[3];",
           true);
}

/**
 * @test Test that an error is returned at parse-time if an invalid register is
 * accessed by an assertion.
 */
TEST_F(CustomCodeTest, ErrorAssertionInvalidQubit) {
  loadCode(3, 0,
           "x q[0];"
           "assert-sup f[3];",
           true);
}

/**
 * @test Test that assertions can be used in custom gates.
 */
TEST_F(CustomCodeTest, AssertionInCustomGate) {
  loadCode(3, 0,
           "gate test q0 {"
           "h q0;"
           "assert-sup q0;"
           "}"
           "test q[0];");
  size_t errors = 0;
  ASSERT_EQ(state->runAll(state, &errors), OK);
  ASSERT_EQ(errors, 0);
}

/**
 * @test Test that assertions can be used in custom gates and reference the
 * correct variable, if variable shadowing occurs.
 */
TEST_F(CustomCodeTest, AssertionInCustomGateShadowing) {
  loadCode(3, 0,
           "gate test q {"
           "h q;"
           "assert-sup q;"
           "}"
           "test q[0];");
  size_t errors = 0;
  ASSERT_EQ(state->runAll(state, &errors), OK);
  ASSERT_EQ(errors, 0);
}

/**
 * @test Test that parsing is successful even if a comment is located at the end
 * of the program.
 */
TEST_F(CustomCodeTest, CommentAtEnd) {
  loadCode(3, 0, "x q[0]; // Comment");
  size_t errors = 0;
  ASSERT_EQ(state->runAll(state, &errors), OK);
  ASSERT_EQ(errors, 0);
  ASSERT_EQ(state->getCurrentInstruction(state), 3);
}

/**
 * @test Test that the `OPENQASM 2.0;` preamble and additional `include`s are
 * handled correctly.
 */
TEST_F(CustomCodeTest, QASMPreamble) {
  loadCode(3, 0, "x q[0]; // Comment", false,
           "OPENQASM 2.0;\ninclude \"qelib1.inc\";\n");
  size_t errors = 0;
  ASSERT_EQ(state->runAll(state, &errors), OK);
  ASSERT_EQ(errors, 0);
  ASSERT_EQ(state->getCurrentInstruction(state), 5);
}

/**
 * @test Test that the simulation works even if a large number of qubits is
 * used.
 */
TEST_F(CustomCodeTest, LargeProgram) {
  loadCode(20, 0, "x q[0]; cx q[0], q[1];");
  size_t errors = 0;
  ASSERT_EQ(state->runAll(state, &errors), OK);
  ASSERT_EQ(errors, 0);
  ASSERT_EQ(state->getCurrentInstruction(state), 4);
}

/**
 * @test Test that a "packed" instruction referencing a full register at once is
 * still correctly treated as a data dependency.
 */
TEST_F(CustomCodeTest, CollectiveGateAsDependency) {
  loadCode(2, 0, "x q; barrier q[0];");
  auto* diagnosis = state->getDiagnostics(state);
  std::array<bool, 4> dependencies{};
  ASSERT_EQ(
      diagnosis->getDataDependencies(diagnosis, 3, false, dependencies.data()),
      OK);
  ASSERT_EQ(dependencies[0], false);
  ASSERT_EQ(dependencies[1], false);
  ASSERT_EQ(dependencies[2], true);
  ASSERT_EQ(dependencies[3], true);
}

/**
 * @test Test that a "packed" instruction referencing a full register at once is
 * still correctly treated as an interaction.
 */
TEST_F(CustomCodeTest, CollectiveGateAsInteraction) {
  loadCode(1, 0, "qreg p[1]; cx q, p; assert-ent q[0], p[0];");
  ASSERT_EQ(state->runSimulation(state), OK);
  ASSERT_TRUE(state->didAssertionFail(state));

  auto* diagnosis = state->getDiagnostics(state);

  std::array<bool, 2> interactions{};
  ASSERT_EQ(diagnosis->getInteractions(diagnosis, 4, 0, interactions.data()),
            OK);

  ASSERT_TRUE(interactions[0]);
  ASSERT_TRUE(interactions[1]);

  std::array<ErrorCause, 1> causes{};
  ASSERT_EQ(
      diagnosis->potentialErrorCauses(diagnosis, causes.data(), causes.size()),
      1);
  ASSERT_EQ(causes[0].type, ControlAlwaysZero);
  ASSERT_EQ(causes[0].instruction, 3);
}

/**
 * @test Test that the non-zero-control flag correctly overrules the
 * zero-control flag.
 *
 * Because of this, the `cx` instruction in this code should not be considered
 * as an error.
 */
TEST_F(CustomCodeTest, NonZeroControlsInErrorSearch) {
  loadCode(2, 0,
           "gate test q1, q2 { cx q1, q2; } x q[0]; test q[1], q[0]; test "
           "q[0], q[1]; assert-sup q[0];");
  auto* diagnosis = state->getDiagnostics(state);
  ASSERT_EQ(state->runSimulation(state), OK);
  ASSERT_TRUE(state->didAssertionFail(state));
  std::array<ErrorCause, 5> errors{};
  ASSERT_TRUE(diagnosis->potentialErrorCauses(diagnosis, errors.data(),
                                              errors.size()) == 0);
}

/**
 * @test Test that a full register can be named as an argument for an assertion.
 */
TEST_F(CustomCodeTest, RegisterInAssertion) {
  loadCode(3, 0,
           "h q[0]; cx q[0], q[1]; cx q[0], q[2];"
           "assert-ent q;"
           "assert-sup q;"
           "assert-eq 0.9, q { 0.707, 0, 0, 0, 0, 0, 0, 0.707 }");
  size_t errors = 0;
  ASSERT_EQ(state->runAll(state, &errors), OK);
  ASSERT_EQ(errors, 0);
}

/**
 * @test Test that a full register can be named as an argument for an assertion
 * together with other full registers or single qubits.
 */
TEST_F(CustomCodeTest, RegisterInAssertionMixed) {
  loadCode(3, 0,
           "qreg f[1]; qreg p[2];"
           "x q[0]; x f[0]; x p[0];"
           "assert-eq q[0], f { 0, 0, 0, 1 }"
           "assert-eq q[0], p { 0, 0, 0, 1, 0, 0, 0, 0 }"
           "assert-eq f, p { 0, 0, 0, 1, 0, 0, 0, 0 }");
  size_t errors = 0;
  ASSERT_EQ(state->runAll(state, &errors), OK);
  ASSERT_EQ(errors, 0);
}

/**
 * @test Test that register shadowing works correctly even if non-shadowed
 * registers are included in an assertion.
 */
TEST_F(CustomCodeTest, ShadowedRegisterInAssertionMixed) {
  loadCode(3, 0,
           "qreg f[1]; qreg p[2];"
           "x q[0]; x f[0];"
           "gate test q {"
           "  x q;"
           "  assert-eq q, f { 0, 0, 0, 1 }"
           "}"
           "test p[0];");
  size_t errors = 0;
  ASSERT_EQ(state->runAll(state, &errors), OK);
  ASSERT_EQ(errors, 0);
}

/**
 * @test Test the "running example" from the paper.
 */
TEST_F(CustomCodeTest, PaperExampleGrover) {
  loadCode(3, 3,
           "gate oracle q0, q1, q2, flag {"
           "assert-sup q0, q1, q2;"
           "ccz q1, q2, flag;"
           "assert-ent q0, q1, q2;"
           "}"
           "gate diffusion q0, q1, q2 {"
           "h q0; h q1; h q2;"
           "x q0; x q1; x q2;"
           "ccz q0, q1, q2;"
           "x q2; x q1; x q0;"
           "h q2; h q1; h q0;"
           "}"
           "qreg flag[1];"
           "x flag;"
           "oracle q[0], q[1], q[2], flag;"
           "diffusion q[0], q[1], q[2];"
           "assert-eq 0.8, q { 0, 0, 0, 0, 0, 0, 0, 1 }"
           "oracle q[0], q[1], q[2], flag;"
           "diffusion q[0], q[1], q[2];"
           "assert-eq 0.9, q { 0, 0, 0, 0, 0, 0, 0, 1 }",
           false, "OPENQASM 2.0;\ninclude \"qelib1.inc\";\n");

  auto* diagnosis = state->getDiagnostics(state);
  std::array<ErrorCause, 10> causes{};

  ASSERT_EQ(state->runSimulation(state), OK);
  ASSERT_EQ(state->didAssertionFail(state), true);
  ASSERT_EQ(state->getCurrentInstruction(state), 5);
  // We expect no potential errors yet:
  ASSERT_EQ(
      diagnosis->potentialErrorCauses(diagnosis, causes.data(), causes.size()),
      0);

  ASSERT_EQ(state->runSimulation(state), OK);
  ASSERT_EQ(state->didAssertionFail(state), true);
  ASSERT_EQ(state->getCurrentInstruction(state), 7);
  // We expect three potential errors:
  //   2 missing interactions: q0 <-> q1 and q0 <-> q2
  //   1 control always zero: q1 & q2 in instruction 6
  ASSERT_EQ(
      diagnosis->potentialErrorCauses(diagnosis, causes.data(), causes.size()),
      3);
  ASSERT_EQ(causes[0].type, MissingInteraction);
  ASSERT_EQ(causes[0].instruction, 7);
  ASSERT_EQ(causes[1].type, MissingInteraction);
  ASSERT_EQ(causes[1].instruction, 7);
  ASSERT_EQ(causes[2].type, ControlAlwaysZero);
  ASSERT_EQ(causes[2].instruction, 6);

  ASSERT_EQ(state->runSimulation(state), OK);
  ASSERT_EQ(state->didAssertionFail(state), true);
  ASSERT_EQ(state->getCurrentInstruction(state), 28);
  // We expect one potential error: Control always zero in instruction 6
  ASSERT_EQ(
      diagnosis->potentialErrorCauses(diagnosis, causes.data(), causes.size()),
      1);
  ASSERT_EQ(causes[0].type, ControlAlwaysZero);
  ASSERT_EQ(causes[0].instruction, 6);

  ASSERT_EQ(state->runSimulation(state), OK);
  ASSERT_EQ(state->didAssertionFail(state), true);
  ASSERT_EQ(state->getCurrentInstruction(state), 31);
  // We expect no potential errors, as instruction 6 is no longer always 0
  ASSERT_EQ(
      diagnosis->potentialErrorCauses(diagnosis, causes.data(), causes.size()),
      0);

  ASSERT_EQ(state->runSimulation(state), OK);
  ASSERT_EQ(state->didAssertionFail(state), false);
  ASSERT_EQ(state->isFinished(state), true);
}

} // namespace mqt::debugger::test
