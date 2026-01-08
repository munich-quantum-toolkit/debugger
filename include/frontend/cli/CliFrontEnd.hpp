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
 * @file CliFrontEnd.hpp
 * @brief Provides a CLI frontend for the debugger.
 *
 * This file contains the declaration of the CliFrontEnd class, which provides
 * a command-line interface for the debugger.
 */

#pragma once

#include "backend/debug.h"

#include <cstddef>
#include <string>

namespace mqt::debugger {

#define ANSI_BG_YELLOW "\x1b[43m"
#define ANSI_BG_RESET "\x1b[0m"
#define ANSI_COL_GRAY "\x1b[90m"

/**
 * @brief A command-line interface for the debugger.
 *
 * By creating an instance of this class and calling the `run` method, the user
 * can interact with the debugger using a command-line interface.
 */
class CliFrontEnd {
public:
  /**
   * @brief Runs the debugger with the given code and state.
   * @param code The code to run (compatible with the provided
   * `SimulationState`)
   * @param state The state to run the code on
   */
  void run(const char* code, SimulationState* state);

private:
  /**
   * @brief The current code being executed. Used to display the code in the
   * CLI.
   */
  std::string currentCode;

  /**
   * @brief Print the current state of the simulation.
   * @param state The simulation state.
   * @param inspecting The instruction that is currently inspected (or -1ULL if
   * nothing is being inspected).
   * @param codeOnly If true, only the code is displayed, not the state.
   */
  void printState(SimulationState* state, size_t inspecting,
                  bool codeOnly = false);

  /**
   * @brief Initialize the code for running it at a later time.
   */
  void initCode(const char* code);

  /**
   * @brief Output a new code with updated assertions based on the assertion
   * refinement rules.
   * @param state The simulation state.
   */
  void suggestUpdatedAssertions(SimulationState* state);
};

} // namespace mqt::debugger
