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
 * @file test_compilation_statistical_slices.cpp
 * @brief Tests the correctness of the assertion compilation process for
 * assertion programs using statistical slices.
 */

#include "common_fixtures.hpp"
#include "utils_test.hpp"

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

namespace mqt::debugger::test {

using SV = std::vector<std::string>;
using DV = std::vector<double>;

/**
 * @brief A test fixture for testing the compilation of assertion programs using
 * only statistical slices.
 */
class StatisticalSlicesCompilationTest : public CompilationTest {};

/**
 * @brief Tests the compilation of a simple equality assertion using
 * statistical slices, no optimization, with a certain outcome.
 */
TEST_F(StatisticalSlicesCompilationTest,
       StatisticalSingleEqualityCertainNoOpt) {
  loadCode("qreg q[1];\n"
           "x q[0];\n"
           "assert-eq q[0] { 0, 1 }\n");

  PreambleVector preamble;
  preamble.emplace_back(
      std::make_unique<StatEqPreambleEntry>(SV{"test_q0"}, DV{0.0, 1.0}, 1.0));

  checkCompilation(makeSettings(/*opt=*/0, /*slice=*/0),
                   "qreg q[1];\n"
                   "x q[0];\n"
                   "creg test_q0[1];\n"
                   "measure q[0] -> test_q0[0];\n",
                   preamble);

  checkNoCompilation(makeSettings(/*opt=*/0, /*slice=*/1));
}

/**
 * @brief Tests the compilation of a simple equality assertion using
 * statistical slices, no optimization, with an uncertain outcome.
 */
TEST_F(StatisticalSlicesCompilationTest,
       StatisticalSingleEqualityUncertainNoOpt) {
  loadCode("qreg q[1];\n"
           "h q[0];\n"
           "assert-eq 0.9, q[0] { 0.707, 0.707 }\n");

  PreambleVector preamble;
  preamble.emplace_back(std::make_unique<StatEqPreambleEntry>(
      SV{"test_q0"}, DV{0.499849, 0.499849}, 0.9));

  checkCompilation(makeSettings(0, 0),
                   "qreg q[1];\n"
                   "h q[0];\n"
                   "creg test_q0[1];\n"
                   "measure q[0] -> test_q0[0];\n",
                   preamble);
}

/**
 * @brief Tests the compilation of an equality assertion on an entangled
 * bell pair using statistical slices, no optimization, with an uncertain
 * outcome.
 */
TEST_F(StatisticalSlicesCompilationTest, StatisticalTwoQubitEqualityNoOpt) {
  loadCode("qreg q[2];\n"
           "h q[0];\n"
           "cx q[0], q[1];\n"
           "assert-eq 0.9, q[0], q[1] { 0.707, 0, 0, 0.707 }\n");

  PreambleVector preamble;
  preamble.emplace_back(std::make_unique<StatEqPreambleEntry>(
      SV{"test_q0", "test_q1"}, DV{0.499849, 0, 0, 0.499849}, 0.9));

  checkCompilation(makeSettings(0, 0),
                   "qreg q[2];\n"
                   "h q[0];\n"
                   "cx q[0], q[1];\n"
                   "creg test_q0[1];\n"
                   "creg test_q1[1];\n"
                   "measure q[0] -> test_q0[0];\n"
                   "measure q[1] -> test_q1[0];\n",
                   preamble);
}

/**
 * @brief Tests the compilation of a two directly consecutive equality
 * assertions using statistical slices, no optimization, with an uncertain
 * outcome.
 */
TEST_F(StatisticalSlicesCompilationTest,
       StatisticalConsecutiveMultiSliceEqualityNoOpt) {
  loadCode("qreg q[1];\n"
           "h q[0];\n"
           "assert-eq 0.9, q[0] { 0.707, 0.707 }\n"
           "assert-eq 0.9, q[0] { 0.707, 0.707 }\n");

  PreambleVector preamble;
  preamble.emplace_back(std::make_unique<StatEqPreambleEntry>(
      SV{"test_q0"}, DV{0.499849, 0.499849}, 0.9));
  checkCompilation(makeSettings(0, 0),
                   "qreg q[1];\n"
                   "h q[0];\n"
                   "creg test_q0[1];\n"
                   "measure q[0] -> test_q0[0];\n",
                   preamble);

  checkCompilation(makeSettings(0, 1),
                   "qreg q[1];\n"
                   "h q[0];\n"
                   "creg test_q0[1];\n"
                   "measure q[0] -> test_q0[0];\n",
                   preamble);

  checkNoCompilation(makeSettings(0, 2));
}

/**
 * @brief Tests the compilation of a two non-consecutive equality assertions
 * using statistical slices, no optimization, with an uncertain outcome.
 */
TEST_F(StatisticalSlicesCompilationTest,
       StatisticalNonConsecutiveMultiSliceEqualityNoOpt) {
  loadCode("qreg q[1];\n"
           "x q[0];\n"
           "assert-eq q[0] { 0, 1 }\n"
           "x q[0];\n"
           "assert-eq q[0] { 1, 0 }\n");

  PreambleVector preamble1;
  preamble1.emplace_back(
      std::make_unique<StatEqPreambleEntry>(SV{"test_q0"}, DV{0, 1}, 1.0));
  checkCompilation(makeSettings(0, 0),
                   "qreg q[1];\n"
                   "x q[0];\n"
                   "creg test_q0[1];\n"
                   "measure q[0] -> test_q0[0];\n",
                   preamble1);

  PreambleVector preamble2;
  preamble2.emplace_back(
      std::make_unique<StatEqPreambleEntry>(SV{"test_q0"}, DV{1, 0}, 1.0));
  checkCompilation(makeSettings(0, 1),
                   "qreg q[1];\n"
                   "x q[0];\n"
                   "x q[0];\n"
                   "creg test_q0[1];\n"
                   "measure q[0] -> test_q0[0];\n",
                   preamble2);

  checkNoCompilation(makeSettings(0, 2));
}

/**
 * @brief Tests the compilation of two directly consecutive equality
 * assertions using statistical slices, with optimization when assertions are
 * equal.
 *
 * The second assertion should not be included in the compiled code as it is
 * removed.
 */
TEST_F(StatisticalSlicesCompilationTest,
       StatisticalConsecutiveMultiSliceEqualityWithOpt) {
  loadCode("qreg q[1];\n"
           "x q[0];\n"
           "assert-eq q[0] { 0, 1 }\n"
           "assert-eq q[0] { 0, 1 }\n");

  PreambleVector preamble;
  preamble.emplace_back(
      std::make_unique<StatEqPreambleEntry>(SV{"test_q0"}, DV{0, 1}, 1.0));
  checkCompilation(makeSettings(1, 0),
                   "qreg q[1];\n"
                   "x q[0];\n"
                   "creg test_q0[1];\n"
                   "measure q[0] -> test_q0[0];\n",
                   preamble);

  checkNoCompilation(makeSettings(1, 1));
}

/**
 * @brief Tests the compilation of two directly consecutive equality
 * assertions using statistical slices, with optimization when the second
 * assertion is a subset of the first.
 *
 * The second assertion should not be included in the compiled code as it is
 * removed.
 */
TEST_F(StatisticalSlicesCompilationTest,
       StatisticalConsecutiveMultiSliceEqualitySmallerScopeOpt) {
  loadCode("qreg q[2];\n"
           "x q[0];\n"
           "assert-eq q[0], q[1] { 0, 1, 0, 0 }\n"
           "assert-eq q[0] { 0, 1 }\n");

  checkNoCompilation(makeSettings(1, 1));
}

/**
 * @brief Tests the compilation of two directly consecutive equality
 * assertions using statistical slices, with optimization when the assertions
 * are not exactly equal but similarity thresholds are still met.
 *
 * The second assertion should not be included in the compiled code as it is
 * removed.
 */
TEST_F(StatisticalSlicesCompilationTest,
       StatisticalConsecutiveMultiSliceEqualityImpreciseOpt) {
  loadCode("qreg q[1];\n"
           "x q[0];\n"
           "assert-eq 0.9, q[0] { 0, 0.9 }\n"
           "assert-eq 0.8, q[0] { 0, 1 }\n");

  checkNoCompilation(makeSettings(1, 1));
}

/**
 * @brief Tests the compilation of two directly consecutive equality
 * assertions using statistical slices, with optimization when similarity
 * thresholds do not allow for the second assertion to be removed.
 */
TEST_F(StatisticalSlicesCompilationTest,
       StatisticalConsecutiveMultiSliceEqualityWithOptDifferentSimilarity) {
  loadCode("qreg q[1];\n"
           "x q[0];\n"
           "assert-eq 0.9, q[0] { 0, 1 }\n"
           "assert-eq 0.99, q[0] { 0, 1 }\n");

  PreambleVector preamble;
  preamble.emplace_back(
      std::make_unique<StatEqPreambleEntry>(SV{"test_q0"}, DV{0, 1}, 0.99));
  checkCompilation(makeSettings(1, 1),
                   "qreg q[1];\n"
                   "x q[0];\n"
                   "creg test_q0[1];\n"
                   "measure q[0] -> test_q0[0];\n",
                   preamble);
}

/**
 * @brief Tests the compilation of two directly consecutive equality
 * assertions using statistical slices, with optimization when different target
 * qubits do not allow for the second assertion to be removed.
 */
TEST_F(StatisticalSlicesCompilationTest,
       StatisticalConsecutiveMultiSliceEqualityWithOptDifferentTargets) {
  loadCode("qreg q[2];\n"
           "x q[0];\n"
           "assert-eq q[0] { 0, 1 }\n"
           "assert-eq q[0], q[1] { 0, 0, 1, 0 }\n");

  PreambleVector preamble;
  preamble.emplace_back(std::make_unique<StatEqPreambleEntry>(
      SV{"test_q0", "test_q1"}, DV{0, 0, 1, 0}, 1.0));
  checkCompilation(makeSettings(1, 1),
                   "qreg q[2];\n"
                   "x q[0];\n"
                   "creg test_q0[1];\n"
                   "creg test_q1[1];\n"
                   "measure q[0] -> test_q0[0];\n"
                   "measure q[1] -> test_q1[0];\n",
                   preamble);
}

/**
 * @brief Tests the compilation of a simple superposition assertion using
 * statistical slices and no optimization.
 */
TEST_F(StatisticalSlicesCompilationTest, StatisticalSingleSuperpositionNoOpt) {
  loadCode("qreg q[1];\n"
           "h q[0];\n"
           "assert-sup q[0];\n");

  PreambleVector preamble;
  preamble.emplace_back(std::make_unique<StatSupPreambleEntry>(SV{"test_q0"}));

  checkCompilation(makeSettings(0, 0),
                   "qreg q[1];\n"
                   "h q[0];\n"
                   "creg test_q0[1];\n"
                   "measure q[0] -> test_q0[0];\n",
                   preamble);

  checkNoCompilation(makeSettings(0, 1));
}

/**
 * @brief Tests the compilation of a 2-qubit superposition assertion using
 * statistical slices and no optimization.
 */
TEST_F(StatisticalSlicesCompilationTest,
       StatisticalMultiQubitSuperpositionNoOpt) {
  loadCode("qreg q[2];\n"
           "h q[0];\n"
           "assert-sup q[0], q[1];\n");

  PreambleVector preamble;
  preamble.emplace_back(
      std::make_unique<StatSupPreambleEntry>(SV{"test_q0", "test_q1"}));

  checkCompilation(makeSettings(0, 0),
                   "qreg q[2];\n"
                   "h q[0];\n"
                   "creg test_q0[1];\n"
                   "creg test_q1[1];\n"
                   "measure q[0] -> test_q0[0];\n"
                   "measure q[1] -> test_q1[0];\n",
                   preamble);

  checkNoCompilation(makeSettings(0, 1));
}

/**
 * @brief Tests the compilation of two consecutive superposition assertions
 * using statistical slices and no optimization.
 */
TEST_F(StatisticalSlicesCompilationTest,
       StatisticalMultipleSuperpositionNoOpt) {
  loadCode("qreg q[2];\n"
           "h q[0];\n"
           "h q[1];\n"
           "assert-sup q[0];\n"
           "assert-sup q[1];\n");

  PreambleVector preamble1;
  preamble1.emplace_back(std::make_unique<StatSupPreambleEntry>(SV{"test_q0"}));

  checkCompilation(makeSettings(0, 0),
                   "qreg q[2];\n"
                   "h q[0];\n"
                   "h q[1];\n"
                   "creg test_q0[1];\n"
                   "measure q[0] -> test_q0[0];\n",
                   preamble1);

  PreambleVector preamble2;
  preamble2.emplace_back(std::make_unique<StatSupPreambleEntry>(SV{"test_q1"}));
  checkCompilation(makeSettings(0, 1),
                   "qreg q[2];\n"
                   "h q[0];\n"
                   "h q[1];\n"
                   "creg test_q1[1];\n"
                   "measure q[1] -> test_q1[0];\n",
                   preamble2);

  checkNoCompilation(makeSettings(0, 2));
}

/**
 * @brief Tests the compilation of two consecutive superposition assertions when
 * the second assertion can be trivially canceled out by the first one.
 */
TEST_F(StatisticalSlicesCompilationTest,
       StatisticalSuperpositionOptIncludedInSup) {
  loadCode("qreg q[1];\n"
           "h q[0];\n"
           "assert-sup q[0];\n"
           "assert-sup q[0];\n");

  PreambleVector preamble;
  preamble.emplace_back(std::make_unique<StatSupPreambleEntry>(SV{"test_q0"}));

  checkCompilation(makeSettings(1, 0),
                   "qreg q[1];\n"
                   "h q[0];\n"
                   "creg test_q0[1];\n"
                   "measure q[0] -> test_q0[0];\n",
                   preamble);

  checkNoCompilation(makeSettings(1, 1));
}

/**
 * @brief Tests the compilation of two superposition assertions when the second
 * assertion can be canceled out by the first one after applying commutation
 * rules.
 */
TEST_F(StatisticalSlicesCompilationTest,
       StatisticalSuperpositionOptIncludedInSupWithCommutation) {
  loadCode("qreg q[1];\n"
           "h q[0];\n"
           "assert-sup q[0];\n"
           "x q[0];\n"
           "assert-sup q[0];\n");

  PreambleVector preamble;
  preamble.emplace_back(std::make_unique<StatSupPreambleEntry>(SV{"test_q0"}));

  checkCompilation(makeSettings(1, 0),
                   "qreg q[1];\n"
                   "h q[0];\n"
                   "creg test_q0[1];\n"
                   "measure q[0] -> test_q0[0];\n",
                   preamble);

  checkNoCompilation(makeSettings(1, 1));
}

/**
 * @brief Tests the compilation of two superposition assertions when the second
 * assertion can be canceled out by the first one as its target qubits
 * are a superset of the first assertion's target qubits.
 */
TEST_F(StatisticalSlicesCompilationTest,
       StatisticalSuperpositionOptIncludedInSupSuperset) {
  loadCode("qreg q[2];\n"
           "h q[0];\n"
           "assert-sup q[0];\n"
           "x q[0];\n"
           "assert-sup q[0], q[1];\n");

  PreambleVector preamble;
  preamble.emplace_back(std::make_unique<StatSupPreambleEntry>(SV{"test_q0"}));

  checkCompilation(makeSettings(1, 0),
                   "qreg q[2];\n"
                   "h q[0];\n"
                   "creg test_q0[1];\n"
                   "measure q[0] -> test_q0[0];\n",
                   preamble);

  checkNoCompilation(makeSettings(1, 1));
}

/**
 * @brief Tests the compilation of two superposition assertions when the second
 * assertion cannot be canceled out by the first one as its target qubits
 * are not a full superset of the first assertion's target qubits.
 */
TEST_F(StatisticalSlicesCompilationTest,
       StatisticalSuperpositionOptNotFullyIncludedInSupSuperset) {
  loadCode("qreg q[3];\n"
           "h q[0];\n"
           "assert-sup q[0], q[2];\n"
           "assert-sup q[0], q[1];\n");

  PreambleVector preamble1;
  preamble1.emplace_back(
      std::make_unique<StatSupPreambleEntry>(SV{"test_q0", "test_q2"}));

  checkCompilation(makeSettings(1, 0),
                   "qreg q[3];\n"
                   "h q[0];\n"
                   "creg test_q0[1];\n"
                   "creg test_q2[1];\n"
                   "measure q[0] -> test_q0[0];\n"
                   "measure q[2] -> test_q2[0];\n",
                   preamble1);

  PreambleVector preamble2;
  preamble2.emplace_back(
      std::make_unique<StatSupPreambleEntry>(SV{"test_q0", "test_q1"}));

  checkCompilation(makeSettings(1, 1),
                   "qreg q[3];\n"
                   "h q[0];\n"
                   "creg test_q0[1];\n"
                   "creg test_q1[1];\n"
                   "measure q[0] -> test_q0[0];\n"
                   "measure q[1] -> test_q1[0];\n",
                   preamble2);
}

/**
 * @brief Tests the compilation of two superposition assertions when the second
 * assertion cannot be canceled out by the first one because there is a
 * non-commuting gate between them.
 */
TEST_F(
    StatisticalSlicesCompilationTest,
    StatisticalSuperpositionOptIncludedInSupSupersetButCommutationNotPossible) {
  loadCode("qreg q[2];\n"
           "h q[0];\n"
           "assert-sup q[0];\n"
           "h q[0];\n"
           "assert-sup q[0], q[1];\n");

  PreambleVector preamble1;
  preamble1.emplace_back(std::make_unique<StatSupPreambleEntry>(SV{"test_q0"}));

  checkCompilation(makeSettings(1, 0),
                   "qreg q[2];\n"
                   "h q[0];\n"
                   "creg test_q0[1];\n"
                   "measure q[0] -> test_q0[0];\n",
                   preamble1);

  PreambleVector preamble2;
  preamble2.emplace_back(
      std::make_unique<StatSupPreambleEntry>(SV{"test_q0", "test_q1"}));

  checkCompilation(makeSettings(1, 1),
                   "qreg q[2];\n"
                   "h q[0];\n"
                   "h q[0];\n"
                   "creg test_q0[1];\n"
                   "creg test_q1[1];\n"
                   "measure q[0] -> test_q0[0];\n"
                   "measure q[1] -> test_q1[0];\n",
                   preamble2);

  checkNoCompilation(makeSettings(1, 2));
}

/**
 * @brief Tests the compilation of two superposition assertions when the second
 * assertion cannot be canceled out by the first one because there is a
 * non-commuting gate between them, even though it is unrelated.
 */
TEST_F(
    StatisticalSlicesCompilationTest,
    StatisticalSuperpositionOptIncludedInSupSupersetButCommutationNotPossibleButUnrelated) {
  loadCode("qreg q[2];\n"
           "h q[0];\n"
           "assert-sup q[0];\n"
           "h q[1];\n"
           "assert-sup q[0], q[1];\n");

  PreambleVector preamble1;
  preamble1.emplace_back(std::make_unique<StatSupPreambleEntry>(SV{"test_q0"}));

  checkCompilation(makeSettings(1, 0),
                   "qreg q[2];\n"
                   "h q[0];\n"
                   "creg test_q0[1];\n"
                   "measure q[0] -> test_q0[0];\n",
                   preamble1);

  PreambleVector preamble2;
  preamble2.emplace_back(
      std::make_unique<StatSupPreambleEntry>(SV{"test_q0", "test_q1"}));

  checkCompilation(makeSettings(1, 1),
                   "qreg q[2];\n"
                   "h q[0];\n"
                   "h q[1];\n"
                   "creg test_q0[1];\n"
                   "creg test_q1[1];\n"
                   "measure q[0] -> test_q0[0];\n"
                   "measure q[1] -> test_q1[0];\n",
                   preamble2);

  checkNoCompilation(makeSettings(1, 2));
}

/**
 * @brief Tests the compilation of an equality assertion followed by a
 * superposition assertions when the second assertion can be canceled out by the
 * first one.
 */
TEST_F(StatisticalSlicesCompilationTest,
       StatisticalSuperpositionOptIncludedInEq) {
  loadCode("qreg q[1];\n"
           "h q[0];\n"
           "assert-eq 0.9, q[0] { 0.707, 0.707 }\n"
           "x q[0];\n"
           "assert-sup q[0];\n");

  PreambleVector preamble;
  preamble.emplace_back(std::make_unique<StatEqPreambleEntry>(
      SV{"test_q0"}, DV{0.499849, 0.499849}, 0.9));

  checkCompilation(makeSettings(1, 0),
                   "qreg q[1];\n"
                   "h q[0];\n"
                   "creg test_q0[1];\n"
                   "measure q[0] -> test_q0[0];\n",
                   preamble);

  checkNoCompilation(makeSettings(1, 1));
}

/**
 * @brief Tests the compilation of an equality assertion followed by a
 * superposition assertions when the second assertion can be canceled out by the
 * first one as a subset of it was in a superposition in the first assertion.
 */
TEST_F(StatisticalSlicesCompilationTest,
       StatisticalSuperpositionOptSubIncludedInEq) {
  loadCode("qreg q[3];\n"
           "h q[0];\n"
           "assert-eq 0.9, q[0], q[1] { 0.707, 0.707, 0, 0 }\n"
           "x q[0];\n"
           "assert-sup q[0], q[2];\n");

  PreambleVector preamble;
  preamble.emplace_back(std::make_unique<StatEqPreambleEntry>(
      SV{"test_q0", "test_q1"}, DV{0.499849, 0.499849, 0, 0}, 0.9));

  checkCompilation(makeSettings(1, 0),
                   "qreg q[3];\n"
                   "h q[0];\n"
                   "creg test_q0[1];\n"
                   "creg test_q1[1];\n"
                   "measure q[0] -> test_q0[0];\n"
                   "measure q[1] -> test_q1[0];\n",
                   preamble);

  checkNoCompilation(makeSettings(1, 1));
}

/**
 * @brief Tests the compilation of an equality assertion followed by a
 * superposition assertions when the second assertion cannot be canceled out by
 * the first one as their intersection is was not in a superposition in the
 * first assertion.
 */
TEST_F(StatisticalSlicesCompilationTest,
       StatisticalSuperpositionOptSubNotEntangledInEq) {
  loadCode("qreg q[3];\n"
           "h q[1];\n"
           "assert-eq 0.9, q[0], q[1] { 0.707, 0, 0.707, 0 }\n"
           "x q[0];\n"
           "assert-sup q[0], q[2];\n");

  PreambleVector preamble1;
  preamble1.emplace_back(std::make_unique<StatEqPreambleEntry>(
      SV{"test_q0", "test_q1"}, DV{0.499849, 0, 0.499849, 0}, 0.9));

  checkCompilation(makeSettings(1, 0),
                   "qreg q[3];\n"
                   "h q[1];\n"
                   "creg test_q0[1];\n"
                   "creg test_q1[1];\n"
                   "measure q[0] -> test_q0[0];\n"
                   "measure q[1] -> test_q1[0];\n",
                   preamble1);

  PreambleVector preamble2;
  preamble2.emplace_back(
      std::make_unique<StatSupPreambleEntry>(SV{"test_q0", "test_q2"}));

  checkCompilation(makeSettings(1, 1),
                   "qreg q[3];\n"
                   "h q[1];\n"
                   "x q[0];\n"
                   "creg test_q0[1];\n"
                   "creg test_q2[1];\n"
                   "measure q[0] -> test_q0[0];\n"
                   "measure q[2] -> test_q2[0];\n",
                   preamble2);

  checkNoCompilation(makeSettings(1, 2));
}

/**
 * @brief Tests the compilation of an equality assertion followed by an
 * entanglement assertions when the second assertion can be canceled out by the
 * first one.
 */
TEST_F(StatisticalSlicesCompilationTest,
       StatisticalEntanglementOptIncludedInEq) {
  loadCode(
      "qreg q[2];\n"
      "h q[0];\n"
      "cx q[0], q[1];\n"
      "assert-eq 0.99999, q[0], q[1] { 0.7071067812, 0, 0, 0.7071067812 }\n"
      "x q[0];\n"
      "assert-ent q[0], q[1];\n");

  PreambleVector preamble;
  preamble.emplace_back(std::make_unique<StatEqPreambleEntry>(
      SV{"test_q0", "test_q1"}, DV{0.5, 0, 0, 0.5}, 0.99999));

  checkCompilation(makeSettings(1, 0),
                   "qreg q[2];\n"
                   "h q[0];\n"
                   "cx q[0], q[1];\n"
                   "creg test_q0[1];\n"
                   "creg test_q1[1];\n"
                   "measure q[0] -> test_q0[0];\n"
                   "measure q[1] -> test_q1[0];\n",
                   preamble);

  checkNoCompilation(makeSettings(1, 1));
}

/**
 * @brief Tests the compilation of two equality assertions that can be
 * combined into one execution at opt-level 2.
 */
TEST_F(StatisticalSlicesCompilationTest, StatisticalEqualityOptCombined) {
  loadCode("qreg q[3];\n"
           "x q[0];\n"
           "assert-eq q[0], q[1] { 0, 1, 0, 0 }\n"
           "assert-eq q[2] { 0, 1 }\n");

  PreambleVector preamble;
  preamble.emplace_back(std::make_unique<StatEqPreambleEntry>(
      SV{"test_q0", "test_q1"}, DV{0, 1, 0, 0}, 1));
  preamble.emplace_back(
      std::make_unique<StatEqPreambleEntry>(SV{"test_q2"}, DV{0, 1}, 1));

  checkCompilation(makeSettings(2, 0),
                   "qreg q[3];\n"
                   "x q[0];\n"
                   "creg test_q0[1];\n"
                   "creg test_q1[1];\n"
                   "measure q[0] -> test_q0[0];\n"
                   "measure q[1] -> test_q1[0];\n"
                   "creg test_q2[1];\n"
                   "measure q[2] -> test_q2[0];\n",
                   preamble);

  checkNoCompilation(makeSettings(2, 1));
}

/**
 * @brief Tests the compilation of two assertions that can be
 * combined into one execution at opt-level 2 with an instruction between.
 */
TEST_F(StatisticalSlicesCompilationTest,
       StatisticalOptCombinedWithOperationBetween) {
  loadCode("qreg q[2];\n"
           "h q[0];\n"
           "assert-sup q[0];\n"
           "h q[0];\n"
           "assert-sup q[1];\n");

  PreambleVector preamble;
  preamble.emplace_back(std::make_unique<StatSupPreambleEntry>(SV{"test_q0"}));
  preamble.emplace_back(std::make_unique<StatSupPreambleEntry>(SV{"test_q1"}));

  checkCompilation(makeSettings(2, 0),
                   "qreg q[2];\n"
                   "h q[0];\n"
                   "creg test_q0[1];\n"
                   "measure q[0] -> test_q0[0];\n"
                   "h q[0];\n"
                   "creg test_q1[1];\n"
                   "measure q[1] -> test_q1[0];\n",
                   preamble);

  checkNoCompilation(makeSettings(2, 1));
}

/**
 * @brief Tests the compilation of two assertions that can be
 * combined into one execution at opt-level 2 with an instruction between.
 */
TEST_F(StatisticalSlicesCompilationTest, StatisticalOptCombinedRemeasure) {
  loadCode("qreg q[2];\n"
           "h q[0];\n"
           "h q[1];\n"
           "assert-sup q[0], q[1];\n"
           "assert-sup q[0];\n");

  PreambleVector preamble;
  preamble.emplace_back(
      std::make_unique<StatSupPreambleEntry>(SV{"test_q0", "test_q1"}));
  preamble.emplace_back(std::make_unique<StatSupPreambleEntry>(SV{"test_q0_"}));

  checkCompilation(makeSettings(2, 0),
                   "qreg q[2];\n"
                   "h q[0];\n"
                   "h q[1];\n"
                   "creg test_q0[1];\n"
                   "creg test_q1[1];\n"
                   "measure q[0] -> test_q0[0];\n"
                   "measure q[1] -> test_q1[0];\n"
                   "creg test_q0_[1];\n"
                   "measure q[0] -> test_q0_[0];\n",
                   preamble);

  checkNoCompilation(makeSettings(2, 1));
}

/**
 * @brief Tests the compilation of three assertions, where the first two can be
 * combined into one execution at opt-level 2 with an instruction between.
 */
TEST_F(StatisticalSlicesCompilationTest,
       StatisticalMixedOptCombinedWithBlockingOperationBetween) {
  loadCode("qreg q[2];\n"
           "x q[0];\n"
           "assert-eq q[0] { 0, 1 }\n"
           "h q[0];\n"
           "assert-sup q[1];\n"
           "assert-sup q[0];\n");

  PreambleVector preambles;
  preambles.emplace_back(
      std::make_unique<StatEqPreambleEntry>(SV{"test_q0"}, DV{0, 1}, 1.0));
  preambles.emplace_back(std::make_unique<StatSupPreambleEntry>(SV{"test_q1"}));

  CompilationTest::checkCompilation(makeSettings(2, 0),
                                    "qreg q[2];\n"
                                    "x q[0];\n"
                                    "creg test_q0[1];\n"
                                    "measure q[0] -> test_q0[0];\n"
                                    "h q[0];\n"
                                    "creg test_q1[1];\n"
                                    "measure q[1] -> test_q1[0];\n",
                                    preambles);

  PreambleVector lastPreamble;
  lastPreamble.emplace_back(
      std::make_unique<StatSupPreambleEntry>(SV{"test_q0"}));

  checkCompilation(makeSettings(2, 1),
                   "qreg q[2];\n"
                   "x q[0];\n"
                   "h q[0];\n"
                   "creg test_q0[1];\n"
                   "measure q[0] -> test_q0[0];\n",
                   lastPreamble);

  checkNoCompilation(makeSettings(2, 2));
}

} // namespace mqt::debugger::test
