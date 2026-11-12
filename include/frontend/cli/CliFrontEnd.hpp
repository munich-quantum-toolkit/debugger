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
#include "frontend/cli/Renderer.hpp"

#include <cstddef>
#include <iosfwd>
#include <optional>
#include <set>
#include <string>
#include <string_view>

namespace mqt::debugger {

/**
 * @brief A command-line interface for the debugger.
 *
 * By creating an instance of this class and calling the `run` method, the user
 * can interact with the debugger using a command-line interface.
 */
class CliFrontEnd {
public:
  /**
   * @brief Construct with the output stream to render to.
   * @param out The stream the CLI will render to. Must outlive this object.
   */
  explicit CliFrontEnd(std::ostream& out);

  ~CliFrontEnd() = default;

  CliFrontEnd(const CliFrontEnd&) = delete;
  CliFrontEnd& operator=(const CliFrontEnd&) = delete;
  CliFrontEnd(CliFrontEnd&&) = delete;
  CliFrontEnd& operator=(CliFrontEnd&&) = delete;

  /**
   * @brief Runs the debugger with the given code and state.
   * @param code The code to run (compatible with the provided
   * `SimulationState`)
   * @param state The state to run the code on
   */
  void run(const char* code, SimulationState* state);

private:
  /**
   * @brief The renderer that funnels every CLI write to the configured output
   * stream.
   */
  Renderer renderer;

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
   * @brief Print one full screen:
   * - help table,
   * - source code,
   * - amplitudes table,
   * - assertion warning (if any), and
   * - response of the last command (if any).
   *
   * @param state The simulation state.
   * @param inspecting The instruction being inspected, or `std::nullopt` if
   * nothing.
   * @param response Text shown just above the prompt; empty means nothing to
   * show.
   */
  void printScreen(SimulationState* state, std::optional<size_t> inspecting,
                   std::string_view response);

  /**
   * @brief Print the persistent help bar. Four rows: F-key shortcuts and
   * descriptions on the first two, single-letter aliases on the last two.
   */
  void printHelpBar();

  /**
   * @brief Print the source code with line numbers, breakpoint markers, the
   * current-instruction highlight, and dimming of the lines that are not
   * data-dependencies of the inspected instruction.
   *
   * @param state The simulation state.
   * @param inspecting The instruction being inspected, or `std::nullopt` if
   * nothing.
   */
  void printCode(SimulationState* state, std::optional<size_t> inspecting);

  /**
   * @brief Print the current state as a two-row table: bitstrings on top,
   * their amplitudes on the bottom.
   *
   * Layout matches the persistent help bar: labels in a white brand-style
   * chip, bitstring values in light-blue chips (F-keys row style), amplitude
   * values in dark-blue chips (letters row style). Each column width is the
   * wider of the bitstring and its amplitude string, so both rows align.
   *
   * When the qubit count exceeds what fits in a single row, the table is
   * skipped and a one-line notice pointing to the `state` command is
   * printed instead.
   *
   * @param state The simulation state to query for amplitudes.
   */
  void printAmplitudes(SimulationState* state);

  /**
   * @brief Initialize the code for running it at a later time.
   * @param code The code to remember for later runs.
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
