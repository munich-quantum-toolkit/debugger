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
 * @file common_fixtures.cpp
 * @brief Provides common fixture base classes for other tests.
 */

#pragma once

#include "backend/dd/DDSimDebug.hpp"
#include "backend/dd/DDSimDiagnostics.hpp"
#include "backend/debug.h"
#include "backend/diagnostics.h"
#include "common.h"
#include "common/parsing/Utils.hpp"
#include "utils_test.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <gtest/gtest.h>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace mqt::debugger::test {

/**
 * @brief Fixture that loads custom code.
 *
 * This fixture sets up a DDSimulationState and provides the method
 * `loadCode` to load custom code into the state.
 */
class CustomCodeFixture : public testing::Test {
protected:
  void SetUp() override {
    createDDSimulationState(&ddState);
    state = &ddState.interface;
    diagnostics = state->getDiagnostics(state);
  }

  /**
   * @brief The DDSimulationState to use for testing.
   */
  DDSimulationState ddState;
  /**
   * @brief The DDSimulationState to use for testing.
   */
  Diagnostics* diagnostics;
  /**
   * @brief A reference to the SimulationState interface for easier access.
   */
  SimulationState* state = nullptr;

  /**
   * @brief The full code to load into the state.
   */
  std::string fullCode;

  /**
   * @brief The code provided by the user explicitly.
   */
  std::string userCode;

  /**
   * @brief Add boilerplate code to the given code.
   *
   * This method adds the necessary qreg and creg declarations to the code.
   * @param numQubits The number of qubits to declare.
   * @param numClassics The number of classical bits to declare.
   * @param code The code to add the boilerplate to.
   * @param preamble The preamble to add to the code before loading declaring
   * the registers (e.g., library imports).
   * @return The code with the boilerplate added.
   */
  virtual std::string addBoilerplate(size_t numQubits, size_t numClassics,
                                     const char* code, const char* preamble) {
    numQubits = std::max<size_t>(numQubits, 1);
    numClassics = std::max<size_t>(numClassics, 1);
    std::ostringstream ss;

    ss << preamble;

    ss << "qreg q[" << numQubits << "];\n";
    ss << "creg c[" << numClassics << "];\n";

    ss << code;

    return ss.str();
  }

  /*
   * @brief Load custom code into the state.
   *
   * By default, classical and Quantum registers of the given size are created
   * automatically with the names `c` and `q`. Therefore, the first instruction
   * in the provided code will have instruction index 2, as 0 and 1 are reserved
   * for the registers.\n\n
   *
   * At least one classical and quantum bit will always be created, even if the
   * given number is less than 1.
   *
   * @param numQubits The number of qubits to create.
   * @param numClassics The number of classical bits to create.
   * @param code The code to load into the state.
   * @param shouldFail Asserts whether the code should fail to load (i.e., due
   * to an expected error).
   * @param preamble The preamble to add to the code before loading declaring
   * the registers (e.g., library imports).
   */
  void loadCode(size_t numQubits, size_t numClassics, const char* code,
                bool shouldFail = false, const char* preamble = "") {
    userCode = code;
    fullCode = addBoilerplate(numQubits, numClassics, code, preamble);
    ASSERT_EQ(state->loadCode(state, fullCode.c_str()),
              shouldFail ? ERROR : OK);
  }

  /**
   * @brief Continue execution until the given instruction is reached.
   *
   * Instruction numbers start with 0 for the first instruction defined in the
   * user code.
   *
   * @param instruction The instruction to forward to.
   */
  void forwardTo(size_t instruction) {
    instruction += 2; // Skip the qreg and creg declarations
    size_t currentInstruction = state->getCurrentInstruction(state);
    while (currentInstruction < instruction) {
      state->stepForward(state);
      currentInstruction = state->getCurrentInstruction(state);
    }
  }
};

/**
 * @brief Fixture that allows code to be loaded from a specific file.
 *
 * The file must be located in the `circuits` directory.
 */
class LoadFromFileFixture : public virtual testing::Test {
protected:
  void SetUp() override {
    createDDSimulationState(&ddState);
    state = &ddState.interface;
    diagnostics = state->getDiagnostics(state);
  }

  /**
   * @brief The DDSimulationState to use for testing.
   */
  DDSimulationState ddState;
  /**
   * @brief A reference to the SimulationState interface for easier access.
   */
  SimulationState* state = nullptr;
  /**
   * @brief A reference to the Diagnostics interface for easier access.
   */
  Diagnostics* diagnostics = nullptr;

  /**
   * @brief Load the code from the file with the given name.
   *
   * The given file should be located in the `circuits` directory and use the
   * `.qasm` extension.
   * @param testName The name of the file to load (not including the `circuits`
   * directory path and the extension).
   */
  void loadFromFile(const std::string& testName) {
    const auto code = readFromCircuitsPath(testName);
    state->loadCode(state, code.c_str());
  }

  /**
   * @brief Continue execution until the given instruction is reached.
   *
   * @param instruction The instruction to forward to.
   */
  void forwardTo(size_t instruction) {
    size_t currentInstruction = state->getCurrentInstruction(state);
    while (currentInstruction < instruction) {
      state->stepForward(state);
      currentInstruction = state->getCurrentInstruction(state);
    }
  }
};

/**
 * @brief Fixture for testing the correctness of the compilation process.
 *
 * This fixture sets up a DDSimulationState and provides the method
 * `loadCode` to load custom code into the state. The code is then
 * compiled and the resulting program is checked for correctness.
 */
class CompilationTest : public CustomCodeFixture {

protected:
  std::string addBoilerplate(size_t /*numQubits*/, size_t /*numClassics*/,
                             const char* code,
                             const char* /*preamble*/) override {
    return code;
  }

  /**
   * @brief Creates a new CompilationSettings object for tests.
   * @param opt The optimization level to use.
   * @param slice The slice index to use.
   * @return The created CompilationSettings object.
   */
  static CompilationSettings makeSettings(uint8_t opt, size_t slice) {
    return {
        /*opt=*/opt,
        /*sliceIndex=*/slice,
    };
  }

  /**
   * @brief Add the testing preamble to the given code.
   * @param code The code to add the preamble to.
   * @param preamble The preamble to add to the code.
   * @return The code with the preamble added.
   */
  static std::string addPreamble(const std::string& code,
                                 const PreambleVector& preamble) {
    std::stringstream ss;
    for (const auto& entry : preamble) {
      ss << entry->toString();
    }
    ss << code;
    return ss.str();
  }

  /**
   * @brief Load the given code into the state.
   *
   * Convenience overload for base method that automatically sets unneeded
   * parameters to 0.
   *
   * @param code The code to load.
   */
  void loadCode(const char* code) {
    CustomCodeFixture::loadCode(0, 0, code, false, "");
  }

  /**
   * @brief Display the expected and actual compiled codes side by side.
   * @param expected The expected compiled code.
   * @param actual The actual compiled code.
   */
  static void prettyPrintComparison(const std::string& expected,
                                    const std::string& actual) {
    auto expectedLines = splitString(expected, '\n');
    auto actualLines = splitString(actual, '\n');
    expectedLines.insert(expectedLines.begin(), "");
    actualLines.insert(actualLines.begin(), "");
    expectedLines.insert(expectedLines.begin(), "EXPECTED:");
    actualLines.insert(actualLines.begin(), "ACTUAL:");
    const auto maxLenExpected =
        std::ranges::max_element(expectedLines, [](const std::string& a,
                                                   const std::string& b) {
          return a.size() < b.size();
        })->size();
    const auto maxLenActual =
        std::ranges::max_element(actualLines, [](const std::string& a,
                                                 const std::string& b) {
          return a.size() < b.size();
        })->size();
    const auto lineCount = std::max(expectedLines.size(), actualLines.size());
    std::cout << "\n";
    for (size_t i = 0; i < lineCount; i++) {
      if (i == 1) {
        std::cout << std::setfill('-');
      } else {
        std::cout << std::setfill(' ');
      }
      const auto expectedLine =
          i < expectedLines.size() ? expectedLines[i] : "";
      const auto actualLine = i < actualLines.size() ? actualLines[i] : "";
      std::cout << std::left << std::setw(static_cast<int>(maxLenExpected + 3))
                << expectedLine << " | "
                << std::setw(static_cast<int>(maxLenActual + 3)) << actualLine
                << "\n";
    }
    std::cout << "\n";
  }

public:
  /**
   * @brief Check the compilation of the loaded code with the given settings.
   * @param settings The settings to use for the compilation.
   * @param expected The expected compiled code.
   */
  void checkCompilation(const CompilationSettings& settings,
                        const std::string& expected,
                        const PreambleVector& expectedPreamble) {
    // Compile the code
    const size_t size = state->compile(state, nullptr, settings);
    ASSERT_NE(size, 0) << "Compilation failed";
    std::vector<char> buffer(size);
    const size_t newSize = state->compile(state, buffer.data(), settings);
    ASSERT_EQ(size, newSize) << "Compilation resulted in unexpected size";
    const auto expectedCode = addPreamble(expected, expectedPreamble);
    const auto receivedCode = std::string(buffer.data());
    prettyPrintComparison(expectedCode, receivedCode);
    ASSERT_EQ(expectedCode, receivedCode)
        << "Compilation resulted in unexpected code";
  }

  /**
   * @brief Check whether the compilation of the loaded code fails with the
   * given settings.
   * @param settings The settings to use for the compilation.
   */
  void checkNoCompilation(const CompilationSettings& settings) {
    ASSERT_EQ(state->compile(state, nullptr, settings), 0)
        << "Compilation should have failed";
  }
};

} // namespace mqt::debugger::test
