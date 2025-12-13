/*
 * Copyright (c) 2024 - 2025 Chair for Design Automation, TUM
 * Copyright (c) 2025 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

/**
 * @file test_data_retrieval.cpp
 * @brief Test the functionality of data-retrieval functions in the simulation.
 *
 * This includes the access to classical variables, qubits, and the state
 * vector.
 */

#include "backend/debug.h"
#include "common.h"
#include "common_fixtures.hpp"
#include "utils_test.hpp"

#include <array>
#include <cstddef>
#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace mqt::debugger::test {

/**
 * @brief Fixture for testing the correctness of the debugger for data retrieval
 * operations.
 *
 * This fixture creates a DDSimulationState and loads the code from the file
 * `circuits/classical-storage`.
 */
class DataRetrievalTest : public LoadFromFileFixture {
  void SetUp() override {
    LoadFromFileFixture::SetUp();
    loadFromFile("classical-storage");
  }
};

/**
 * @test Test the correctness of `getNumQubits` method of the debugging
 * interface.r
 */
TEST_F(DataRetrievalTest, GetNumQubits) {
  ASSERT_EQ(state->getNumQubits(state), 4);
}

/**
 * @test Test the correctness of `getNumClassicalVariables` method of the
 * debugging interface.
 */
TEST_F(DataRetrievalTest, GetNumClassicalVariables) {
  ASSERT_EQ(state->getNumClassicalVariables(state), 4);
}

/**
 * @test Test the correctness of amplitude retrieval methods of the debugging
 * interface.
 *
 * This includes the retrieval of amplitudes by index and by bitstring.
 */
TEST_F(DataRetrievalTest, GetAmplitudes) {
  Complex result;

  forwardTo(6);
  ASSERT_EQ(state->getAmplitudeIndex(state, 0, &result), OK);
  ASSERT_TRUE(complexEquality(result, 0.0, 0.0));
  ASSERT_EQ(state->getAmplitudeIndex(state, 3, &result), OK);
  ASSERT_TRUE(complexEquality(result, 1.0, 0.0));
  ASSERT_EQ(state->getAmplitudeBitstring(state, "0011", &result), OK);
  ASSERT_TRUE(complexEquality(result, 1.0, 0.0));
  ASSERT_EQ(state->getAmplitudeBitstring(state, "1011", &result), OK);
  ASSERT_TRUE(complexEquality(result, 0.0, 0.0));

  forwardTo(12);
  ASSERT_EQ(state->getAmplitudeBitstring(state, "1011", &result), OK);
  ASSERT_TRUE(complexEquality(result, -0.707, 0.0));
  ASSERT_EQ(state->getAmplitudeBitstring(state, "0010", &result), OK);
  ASSERT_TRUE(complexEquality(result, 0.707, 0.0));
  ASSERT_EQ(state->getAmplitudeBitstring(state, "1010", &result), OK);
  ASSERT_TRUE(complexEquality(result, 0.0, 0.0));
  ASSERT_EQ(state->getAmplitudeIndex(state, 3, &result), OK);
  ASSERT_TRUE(complexEquality(result, 0.0, 0.0));

  forwardTo(13);
  Complex c1;
  Complex c2;
  ASSERT_EQ(state->getAmplitudeBitstring(state, "0010", &c1), OK);
  ASSERT_EQ(state->getAmplitudeBitstring(state, "1011", &c2), OK);
  ASSERT_TRUE(
      (complexEquality(c1, 0.0, 0.0) && complexEquality(c2, -1.0, 0.0)) ||
      (complexEquality(c1, 1.0, 0.0) && complexEquality(c2, 0.0, 0.0)));
  const size_t baseIndex = complexEquality(c1, 1.0, 0.0) ? 2 : 11;

  forwardTo(14);
  ASSERT_EQ(state->getAmplitudeIndex(state, baseIndex + 4, &result), OK);
  ASSERT_TRUE(complexEquality(result, 0.0, baseIndex == 2 ? 1 : -1.0));
  ASSERT_EQ(state->getAmplitudeIndex(state, baseIndex, &result), OK);
  ASSERT_TRUE(complexEquality(result, 0.0, 0.0));

  forwardTo(15);
  ASSERT_EQ(state->getAmplitudeIndex(state, baseIndex + 2, &result), OK);
  ASSERT_TRUE(complexEquality(result, baseIndex == 2 ? 1.0 : -1.0, 0.0));
  ASSERT_EQ(state->getAmplitudeIndex(state, baseIndex, &result), OK);
  ASSERT_TRUE(complexEquality(result, 0.0, 0.0));
}

/**
 * @test Test the correctness of the `getClassicalVariableName` method of the
 * debugging interface.
 */
TEST_F(DataRetrievalTest, GetClassicalVariableNames) {
  std::array<char, 256> name = {0};
  std::vector<std::string> expectedNames = {"c[0]", "c[1]", "c[2]", "hello[0]"};
  for (size_t i = 0; i < expectedNames.size(); i++) {
    ASSERT_EQ(state->getClassicalVariableName(state, i, name.data()), OK);
    ASSERT_STREQ(name.data(), expectedNames[i].c_str());
  }
}

/**
 * @test Test the correctness of the `getClassicalVariable` method of the
 * debugging interface.
 *
 * This tests access to variables that have not yet been set, variables of
 * different sizes (including single bits), and their effects on the quantum
 * state.
 */
TEST_F(DataRetrievalTest, GetClassicalVariable) {
  Variable v;

  forwardTo(6);
  ASSERT_EQ(state->getClassicalVariable(state, "c[0]", &v), OK);
  ASSERT_TRUE(classicalEquals(v, false));

  forwardTo(7);
  ASSERT_EQ(state->getClassicalVariable(state, "c[0]", &v), OK);
  ASSERT_TRUE(classicalEquals(v, true));
  ASSERT_EQ(state->getClassicalVariable(state, "c[1]", &v), OK);
  ASSERT_TRUE(classicalEquals(v, true));

  forwardTo(10);
  ASSERT_EQ(state->getClassicalVariable(state, "c[2]", &v), OK);
  ASSERT_TRUE(classicalEquals(v, true));

  forwardTo(13);
  ASSERT_EQ(state->getClassicalVariable(state, "hello[0]", &v), OK);
  const bool entangledValue = v.value.boolValue;
  if (entangledValue) {
    Complex c;
    ASSERT_EQ(state->getAmplitudeBitstring(state, "1011", &c), OK);
    ASSERT_TRUE(c.real == -1 || c.real == 1);
  } else {
    Complex c;
    ASSERT_EQ(state->getAmplitudeBitstring(state, "0010", &c), OK);
    ASSERT_TRUE(c.real == -1 || c.real == 1);
  }

  forwardTo(16);
  ASSERT_EQ(state->getClassicalVariable(state, "c[0]", &v), OK);
  ASSERT_TRUE(classicalEquals(v, entangledValue));
}

/**
 * @test Test the correctness of the `getStateVectorFull` method of the
 * debugging interface at different times during execution.
 */
TEST_F(DataRetrievalTest, GetStateVectorFull) {
  std::array<Complex, 16> amplitudes{};
  Statevector sv{4, 16, amplitudes.data()};

  ASSERT_EQ(state->getStateVectorFull(state, &sv), OK);
  ASSERT_TRUE(complexEquality(amplitudes[0], 1.0, 0.0));
  ASSERT_TRUE(complexEquality(amplitudes[1], 0.0, 0.0));

  forwardTo(12);
  ASSERT_EQ(state->getStateVectorFull(state, &sv), OK);
  ASSERT_TRUE(complexEquality(amplitudes[2], 0.707, 0.0));
  ASSERT_TRUE(complexEquality(amplitudes[11], -0.707, 0.0));
}

/**
 * @test Test the correctness of the `getStateVectorSub` method of the debugging
 * interface at different times during execution.
 */
TEST_F(DataRetrievalTest, GetStateVectorSub) {
  std::array<Complex, 4> amplitudes{};
  Statevector sv{2, 4, amplitudes.data()};

  forwardTo(6);
  std::array<size_t, 2> qubits = {0, 1};
  ASSERT_EQ(state->getStateVectorSub(state, 2, qubits.data(), &sv), OK);
  ASSERT_TRUE(complexEquality(amplitudes[3], 1, 0.0));
  ASSERT_TRUE(complexEquality(amplitudes[0], 0.0, 0.0));

  qubits[1] = 2;
  ASSERT_EQ(state->getStateVectorSub(state, 2, qubits.data(), &sv), OK);
  ASSERT_TRUE(complexEquality(amplitudes[3], 0.0, 0.0));
  ASSERT_TRUE(complexEquality(amplitudes[1], 1.0, 0.0));

  forwardTo(11);
  ASSERT_EQ(state->getStateVectorSub(state, 2, qubits.data(), &sv), OK);
  ASSERT_TRUE(complexEquality(amplitudes[0], 0.707, 0.0));
  ASSERT_TRUE(complexEquality(amplitudes[1], -0.707, 0.0));

  qubits[0] = 1;
  ASSERT_EQ(state->getStateVectorSub(state, 2, qubits.data(), &sv), OK);
  ASSERT_TRUE(complexEquality(amplitudes[0], 0.0, 0.0));
  ASSERT_TRUE(complexEquality(amplitudes[1], 1.0, 0.0));
  ASSERT_TRUE(complexEquality(amplitudes[2], 0.0, 0.0));
  ASSERT_TRUE(complexEquality(amplitudes[3], 0.0, 0.0));
}

/**
 * @test Test that an error is returned when trying to access an unknown
 * classical variable.
 */
TEST_F(DataRetrievalTest, GetUnknownClassicalVariable) {
  Variable v;

  forwardTo(6);
  ASSERT_EQ(state->getClassicalVariable(state, "u[0]", &v), ERROR);
}

/**
 * @test Test that an error is returned when trying to access a classical
 * variable at an invalid index.
 */
TEST_F(DataRetrievalTest, GetBadClassicalVariableName) {
  std::array<char, 256> name = {0};
  forwardTo(6);
  ASSERT_EQ(state->getClassicalVariableName(state, 5, name.data()), ERROR);
}

/**
 * @test Test that classical variables can be updated explicitly.
 */
TEST_F(DataRetrievalTest, ChangeClassicalVariableUpdatesValue) {
  Variable initial;
  forwardTo(6);
  ASSERT_EQ(state->getClassicalVariable(state, "c[0]", &initial), OK);
  ASSERT_TRUE(classicalEquals(initial, false));

  VariableValue desired{};
  desired.boolValue = true;
  ASSERT_EQ(state->changeClassicalVariableValue(state, "c[0]", &desired), OK);

  Variable updated;
  ASSERT_EQ(state->getClassicalVariable(state, "c[0]", &updated), OK);
  ASSERT_TRUE(classicalEquals(updated, true));

  desired.boolValue = false;
  ASSERT_EQ(state->changeClassicalVariableValue(state, "c[0]", &desired), OK);
  ASSERT_EQ(state->getClassicalVariable(state, "c[0]", &updated), OK);
  ASSERT_TRUE(classicalEquals(updated, false));
}

/**
 * @test Test that change requests for unknown variables are rejected.
 */
TEST_F(DataRetrievalTest, ChangeClassicalVariableUnknown) {
  VariableValue desired{};
  desired.boolValue = true;
  ASSERT_EQ(
      state->changeClassicalVariableValue(state, "does_not_exist", &desired),
      ERROR);
}

/**
 * @test Test that passing a null value pointer results in an error.
 */
TEST_F(DataRetrievalTest, ChangeClassicalVariableNullValue) {
  forwardTo(6);
  ASSERT_EQ(state->changeClassicalVariableValue(state, "c[0]", nullptr), ERROR);
}

/**
 * @test Test that amplitudes can be updated and the remaining state is
 * rescaled.
 */
TEST_F(DataRetrievalTest, ChangeAmplitudeValueRescalesOtherStates) {
  forwardTo(12);
  const Complex desired{0.5, 0.0};
  ASSERT_EQ(state->changeAmplitudeValue(state, "0010", &desired), OK);

  Complex updated{};
  ASSERT_EQ(state->getAmplitudeBitstring(state, "0010", &updated), OK);
  ASSERT_TRUE(complexEquality(updated, 0.5, 0.0));

  ASSERT_EQ(state->getAmplitudeBitstring(state, "1011", &updated), OK);
  ASSERT_TRUE(complexEquality(updated, -0.866, 0.0));
}

/**
 * @test Test that invalid bitstrings for amplitude updates are rejected.
 */
TEST_F(DataRetrievalTest, ChangeAmplitudeValueRejectsInvalidBitstring) {
  const Complex desired{0.25, 0.0};
  forwardTo(12);
  ASSERT_EQ(state->changeAmplitudeValue(state, "10", &desired), ERROR);
  ASSERT_EQ(state->changeAmplitudeValue(state, "10a1", &desired), ERROR);
}

/**
 * @test Test that sub-normalized targets without residual amplitudes are
 * refused.
 */
TEST_F(DataRetrievalTest, ChangeAmplitudeValueRejectsSubNormalizedVacuum) {
  const Complex desired{0.5, 0.0};
  ASSERT_EQ(state->changeAmplitudeValue(state, "0000", &desired), ERROR);
  ASSERT_EQ(state->changeAmplitudeValue(state, "0000", nullptr), ERROR);
}

} // namespace mqt::debugger::test
