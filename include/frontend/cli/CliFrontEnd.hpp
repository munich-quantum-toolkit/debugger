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
#include <set>
#include <string>
#include <string_view>

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
   * @brief The current code being executed.
   *
   * Used to display the code in the CLI.
   */
  std::string currentCode;

  /**
   * @brief 1-based line numbers that currently carry a breakpoint.
   *
   * Used to paint their gutter numbers on a red background in the source view.
   */
  std::set<size_t> breakpointLines;

  /**
   * @brief Persistent help bar, top row: brand chips plus F-key shortcuts.
   *
   * White background for the brand, light-blue background for the F-keys.
   */
  static constexpr std::string_view HELP_BAR_LINE_1 =
      "\x1b[47m\x1b[30m           \x1b[1mMQT\x1b[22m\x1b[0m"
      "\x1b[47m\x1b[30m|\x1b[1mDebugger\x1b[22m      \x1b[0m"
      "\x1b[48;5;153m\x1b[30m \x1b[1mF5\x1b[22m Run      \x1b[0m"
      "\x1b[48;5;153m\x1b[30m| \x1b[1mF6\x1b[22m Step      \x1b[0m"
      "\x1b[48;5;153m\x1b[30m| \x1b[1mF7\x1b[22m Step over \x1b[0m"
      "\x1b[48;5;153m\x1b[30m| \x1b[1mF9\x1b[22m Run back \x1b[0m"
      "\x1b[48;5;153m\x1b[30m| \x1b[1mF10\x1b[22m Back   \x1b[0m"
      "\x1b[48;5;153m\x1b[30m| \x1b[1mF11\x1b[22m Back over \x1b[0m";

  /**
   * @brief Persistent help bar, bottom row: single-letter aliases.
   *
   * Dark-blue background, white text.
   */
  static constexpr std::string_view HELP_BAR_LINE_2 =
      "\x1b[44m\x1b[97m \x1b[1ma\x1b[22m Assertions \x1b[0m"
      "\x1b[44m\x1b[97m| \x1b[1mb\x1b[22m Break \x1b[3m<N>\x1b[23m \x1b[0m"
      "\x1b[44m\x1b[97m|  \x1b[1md\x1b[22m Diagnose \x1b[0m"
      "\x1b[44m\x1b[97m|  \x1b[1mg\x1b[22m Get \x1b[3m<var>\x1b[23m \x1b[0m"
      "\x1b[44m\x1b[97m|  \x1b[1mi\x1b[22m Inspect   \x1b[0m"
      "\x1b[44m\x1b[97m|  \x1b[1mr\x1b[22m Reset    \x1b[0m"
      "\x1b[44m\x1b[97m|   \x1b[1ms\x1b[22m State  \x1b[0m"
      "\x1b[44m\x1b[97m|   \x1b[1mq\x1b[22m Quit      \x1b[0m";

  /**
   * @brief Print the two-row persistent help bar (F-keys on top, single-letter
   * aliases below).
   */
  static void printHelpBar();

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
   * @brief Print the current state as a two-row table: bitstrings on top,
   * their amplitudes on the bottom.
   *
   * Layout matches the persistent help bar: labels in a white brand-style
   * chip, bitstring values in light-blue chips (F-keys row style), amplitude
   * values in dark-blue chips (letters row style). Each column width is the
   * wider of the bitstring and its amplitude string, so both rows align.
   *
   * @param state The simulation state to query for amplitudes.
   */
  static void printAmplitudes(SimulationState* state);

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
