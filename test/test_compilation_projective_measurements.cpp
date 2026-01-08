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
 * @file test_compilation_projective_measurements.cpp
 * @brief Tests the correctness of the assertion compilation process for
 * assertion programs using projective measurements.
 */

#include "common_fixtures.hpp"
#include "utils_test.hpp"

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

namespace mqt::debugger::test {

using SV = std::vector<std::string>;

/**
 * @brief A test fixture for testing the compilation of assertion programs using
 * only projective measurements.
 */
class ProjectiveMeasurementsCompilationTest : public CompilationTest {};

/**
 * @brief Tests the compilation of a simple equality assertion using
 * projective measurements and no optimization.
 */
TEST_F(ProjectiveMeasurementsCompilationTest, ProjectiveSingleOperation) {
  loadCode("qreg q[1];\n"
           "x q[0];\n"
           "assert-eq q[0] { "
           "qreg p[1];\n"
           "x p[0];\n"
           "}\n");

  PreambleVector preamble;
  preamble.emplace_back(std::make_unique<ProjPreambleEntry>(SV{"test_q0"}));

  checkCompilation(makeSettings(/*opt=*/0, /*slice=*/0),
                   "qreg q[1];\n"
                   "x q[0];\n"
                   "creg test_q0[1];\n"
                   "x q[0];\n"
                   "measure q[0] -> test_q0[0];\n"
                   "x q[0];\n",
                   preamble);

  checkNoCompilation(makeSettings(0, 1));
}

/**
 * @brief Tests the compilation of a equality assertion consisting of multiple
 * instructions using projective measurements and no optimization.
 */
TEST_F(ProjectiveMeasurementsCompilationTest,
       ProjectiveSingleQubitMultipleOperations) {
  loadCode("qreg q[1];\n"
           "h q[0];\n"
           "z q[0];\n"
           "h q[0];\n"
           "assert-eq q[0] { "
           "qreg p[1];\n"
           "x p[0];\n"
           "}\n");

  PreambleVector preamble;
  preamble.emplace_back(std::make_unique<ProjPreambleEntry>(SV{"test_q0"}));

  checkCompilation(makeSettings(/*opt=*/0, /*slice=*/0),
                   "qreg q[1];\n"
                   "h q[0];\n"
                   "z q[0];\n"
                   "h q[0];\n"
                   "creg test_q0[1];\n"
                   "x q[0];\n"
                   "measure q[0] -> test_q0[0];\n"
                   "x q[0];\n",
                   preamble);
}

/**
 * @brief Tests the compilation of a two-qubit equality assertion using
 * projective measurements and no optimization.
 */
TEST_F(ProjectiveMeasurementsCompilationTest,
       ProjectiveMultiQubitMultipleOperations) {
  loadCode("qreg q[2];\n"
           "h q[0];\n"
           "cx q[0], q[1];\n"
           "assert-eq q[0], q[1] { "
           "qreg p[2];\n"
           "h p[1];\n"
           "cx p[1], p[0];\n"
           "}\n");

  PreambleVector preamble;
  preamble.emplace_back(
      std::make_unique<ProjPreambleEntry>(SV{"test_q0", "test_q1"}));

  checkCompilation(makeSettings(/*opt=*/0, /*slice=*/0),
                   "qreg q[2];\n"
                   "h q[0];\n"
                   "cx q[0], q[1];\n"
                   "creg test_q0[1];\n"
                   "creg test_q1[1];\n"
                   "cx q[1], q[0];\n"
                   "h q[1];\n"
                   "measure q[0] -> test_q0[0];\n"
                   "measure q[1] -> test_q1[0];\n"
                   "h q[1];\n"
                   "cx q[1], q[0];\n",
                   preamble);
}

/**
 * @brief Tests the compilation of an equality assertion that considers only a
 * substate of the full state vector, using projective measurements and no
 * optimization.
 */
TEST_F(ProjectiveMeasurementsCompilationTest, ProjectiveSubstateAssertion) {
  loadCode("qreg q[2];\n"
           "x q[1];\n"
           "h q[0];\n"
           "assert-eq q[1] { "
           "qreg p[1];\n"
           "h p[0];\n"
           "z p[0];\n"
           "h p[0];\n"
           "}\n");

  PreambleVector preamble;
  preamble.emplace_back(std::make_unique<ProjPreambleEntry>(SV{"test_q1"}));

  checkCompilation(makeSettings(/*opt=*/0, /*slice=*/0),
                   "qreg q[2];\n"
                   "x q[1];\n"
                   "h q[0];\n"
                   "creg test_q1[1];\n"
                   "h q[1];\n"
                   "z q[1];\n"
                   "h q[1];\n"
                   "measure q[1] -> test_q1[0];\n"
                   "h q[1];\n"
                   "z q[1];\n"
                   "h q[1];\n",
                   preamble);
}

/**
 * @brief Tests the compilation of multiple equality assertions using
 * projective measurements and no optimization.
 */
TEST_F(ProjectiveMeasurementsCompilationTest,
       ProjectiveMultiAssertionSingleOperation) {
  loadCode("qreg q[1];\n"
           "x q[0];\n"
           "assert-eq q[0] { "
           "qreg p[1];\n"
           "x p[0];\n"
           "}\n"
           "assert-eq q[0] { "
           "qreg p[1];\n"
           "h p[0];\n"
           "z p[0];\n"
           "h p[0];\n"
           "}\n");

  PreambleVector preamble;
  preamble.emplace_back(std::make_unique<ProjPreambleEntry>(SV{"test_q0"}));
  preamble.emplace_back(std::make_unique<ProjPreambleEntry>(SV{"test_q0_"}));

  checkCompilation(makeSettings(/*opt=*/2, /*slice=*/0),
                   "qreg q[1];\n"
                   "x q[0];\n"
                   "creg test_q0[1];\n"
                   "x q[0];\n"
                   "measure q[0] -> test_q0[0];\n"
                   "x q[0];\n"
                   "creg test_q0_[1];\n"
                   "h q[0];\n"
                   "z q[0];\n"
                   "h q[0];\n"
                   "measure q[0] -> test_q0_[0];\n"
                   "h q[0];\n"
                   "z q[0];\n"
                   "h q[0];\n",
                   preamble);

  checkNoCompilation(makeSettings(2, 1));
}

/**
 * @brief Tests the compilation of multiple equality assertions with other
 * instructions between them, using projective measurements and no optimization.
 */
TEST_F(ProjectiveMeasurementsCompilationTest,
       ProjectiveMultiAssertionOperationsBetween) {
  loadCode("qreg q[1];\n"
           "x q[0];\n"
           "assert-eq q[0] { "
           "qreg p[1];\n"
           "x p[0];\n"
           "}\n"
           "z q[0];\n"
           "assert-eq q[0] { "
           "qreg p[1];\n"
           "x p[0];\n"
           "z p[0];\n"
           "}\n");

  PreambleVector preamble;
  preamble.emplace_back(std::make_unique<ProjPreambleEntry>(SV{"test_q0"}));
  preamble.emplace_back(std::make_unique<ProjPreambleEntry>(SV{"test_q0_"}));

  checkCompilation(makeSettings(/*opt=*/2, /*slice=*/0),
                   "qreg q[1];\n"
                   "x q[0];\n"
                   "creg test_q0[1];\n"
                   "x q[0];\n"
                   "measure q[0] -> test_q0[0];\n"
                   "x q[0];\n"
                   "z q[0];\n"
                   "creg test_q0_[1];\n"
                   "z q[0];\n"
                   "x q[0];\n"
                   "measure q[0] -> test_q0_[0];\n"
                   "x q[0];\n"
                   "z q[0];\n",
                   preamble);

  checkNoCompilation(makeSettings(2, 1));
}

} // namespace mqt::debugger::test
