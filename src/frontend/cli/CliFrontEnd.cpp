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
 * @file CliFrontEnd.cpp
 * @brief Implementation of the command-line interface front end.
 */
#include "frontend/cli/CliFrontEnd.hpp"

#include "backend/debug.h"
#include "backend/diagnostics.h"
#include "common.h"
#include "frontend/cli/LineEditor.hpp"
#include "frontend/cli/RawModeTerminal.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <memory>
#include <ranges>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace mqt::debugger {

namespace {

size_t boundedStrnlen(const char* data, size_t max) {
  const auto* end = static_cast<const char*>(std::memchr(data, '\0', max));
  return end != nullptr ? static_cast<size_t>(end - data) : max;
}

std::string_view loadResultMessageView(const LoadResult& result) {
  const auto* data = std::data(result.message);
  return {data, boundedStrnlen(data, LOAD_RESULT_MESSAGE_MAX)};
}

/**
 * @brief ANSI escape sequence for resetting the background color.
 *
 * This method clears the terminal screen.
 */
void clearScreen() {
  // Clear the screen using an ANSI escape sequence
  std::cout << "\033[2J\033[1;1H";
}

/// @brief Persistent help bar, top row: brand chips plus F-key shortcuts.
/// White background for the brand, light-blue background for the F-keys.
constexpr std::string_view HELP_BAR_LINE_1 =
    "\x1b[47m\x1b[30m           \x1b[1mMQT\x1b[22m\x1b[0m"
    "\x1b[47m\x1b[30m|\x1b[1mDebugger\x1b[22m      \x1b[0m"
    "\x1b[48;5;153m\x1b[30m \x1b[1mF5\x1b[22m Run      \x1b[0m"
    "\x1b[48;5;153m\x1b[30m| \x1b[1mF6\x1b[22m Step      \x1b[0m"
    "\x1b[48;5;153m\x1b[30m| \x1b[1mF7\x1b[22m Step over \x1b[0m"
    "\x1b[48;5;153m\x1b[30m| \x1b[1mF9\x1b[22m Run back \x1b[0m"
    "\x1b[48;5;153m\x1b[30m| \x1b[1mF10\x1b[22m Back   \x1b[0m"
    "\x1b[48;5;153m\x1b[30m| \x1b[1mF11\x1b[22m Back over \x1b[0m";

/// @brief Persistent help bar, bottom row: single-letter aliases.
/// Dark-blue background, white text.
constexpr std::string_view HELP_BAR_LINE_2 =
    "\x1b[44m\x1b[97m \x1b[1ma\x1b[22m Assertions \x1b[0m"
    "\x1b[44m\x1b[97m| \x1b[1mb\x1b[22m Break \x1b[3m<N>\x1b[23m \x1b[0m"
    "\x1b[44m\x1b[97m|  \x1b[1md\x1b[22m Diagnose \x1b[0m"
    "\x1b[44m\x1b[97m|  \x1b[1mg\x1b[22m Get \x1b[3m<var>\x1b[23m \x1b[0m"
    "\x1b[44m\x1b[97m|  \x1b[1mi\x1b[22m Inspect   \x1b[0m"
    "\x1b[44m\x1b[97m|  \x1b[1mr\x1b[22m Reset    \x1b[0m"
    "\x1b[44m\x1b[97m|   \x1b[1ms\x1b[22m State  \x1b[0m"
    "\x1b[44m\x1b[97m|   \x1b[1mq\x1b[22m Quit      \x1b[0m";

/**
 * @brief Print the two-row persistent help bar.
 */
void printHelpBar() {
  std::cout << HELP_BAR_LINE_1 << "\n" << HELP_BAR_LINE_2 << "\n";
}

/**
 * @brief Prefix each source line in @p text with a right-aligned 1-based
 * line number.
 *
 * The gutter width is picked from the total line count so all separators
 * line up.
 * Preserves any ANSI color codes already present in @p text.
 * Only `'\n'` characters trigger a new gutter, so highlights that span newlines
 * still render correctly.
 */
std::string addLineNumbers(std::string_view text) {
  while (!text.empty() && text.back() == '\n') {
    text.remove_suffix(1);
  }
  if (text.empty()) {
    return {};
  }

  auto lines = text | std::views::split('\n');
  const auto lineCount = static_cast<size_t>(std::ranges::distance(lines));
  const auto gutterWidth = static_cast<int>(std::to_string(lineCount).size());

  std::ostringstream oss;
  size_t lineNum = 1;
  for (const auto& line : lines) {
    const std::string_view sv{line.begin(), line.end()};
    oss << std::setw(gutterWidth) << lineNum++ << " " << sv << "\n";
  }
  return oss.str();
}

/**
 * @brief Get all possible bit strings for a given number of qubits.
 * @param numQubits The number of qubits.
 * @return The list of bit strings.
 */
std::vector<std::string> getBitStrings(size_t numQubits) {
  std::vector<std::string> bitStrings;
  for (size_t i = 0; i < (1ULL << numQubits); i++) {
    std::string bitString;
    for (size_t j = 0; j < numQubits; j++) {
      bitString.insert(bitString.begin(), (i & (1 << j)) > 0 ? '1' : '0');
    }
    bitStrings.push_back(bitString);
  }
  return bitStrings;
}

} // namespace

void CliFrontEnd::initCode(const char* code) { currentCode = code; }

void CliFrontEnd::run(const char* code, SimulationState* state) {
  initCode(code);

  const auto result = state->loadCode(state, code);
  state->resetSimulation(state);
  if (result.status != LOAD_OK) {
    const auto messageView = loadResultMessageView(result);
    if (!messageView.empty()) {
      std::cout << "Error loading code: " << messageView << "\n";
    } else {
      std::cout << "Error loading code\n";
    }
    return;
  }

  const RawModeTerminal rawMode;
  LineEditor editor{std::cin, std::cout, "Enter command: "};
  editor.bindKey("15~", "run");       // F5
  editor.bindKey("17~", "step");      // F6
  editor.bindKey("18~", "step over"); // F7
  editor.bindKey("20~", "run back");  // F9
  editor.bindKey("21~", "back");      // F10
  editor.bindKey("23~", "back over"); // F11

  std::string command;
  std::string response;
  size_t inspecting = -1ULL;

  while (command != "quit" && command != "q") {
    clearScreen();
    printHelpBar();
    printState(state, inspecting, state->getNumQubits(state) >= 7);
    if (!response.empty()) {
      std::cout << response << "\n";
      response.clear();
    }

    auto line = editor.readLine();
    if (!line.has_value()) {
      break;
    }
    command = std::move(*line);
    if (!command.empty()) {
      editor.addToHistory(command);
    }
    if (command == "run") {
      state->runSimulation(state);
    } else if (command == "run back") {
      state->runSimulationBackward(state);
    } else if (command == "step" || command.empty()) {
      state->stepForward(state);
    } else if (command == "step over") {
      state->stepOverForward(state);
    } else if (command == "back") {
      state->stepBackward(state);
    } else if (command == "back over") {
      state->stepOverBackward(state);
    } else if (command == "assertions" || command == "a") {
      suggestUpdatedAssertions(state);
    } else if (command.starts_with("breakpoint ") ||
               command.starts_with("b ")) {
      const auto param = command.substr(command.find(' ') + 1);
      const auto* const paramBegin = std::to_address(param.begin());
      const auto* const paramEnd = std::to_address(param.end());
      size_t position = 0;
      const auto [ptr, ec] = std::from_chars(paramBegin, paramEnd, position);
      if (ec != std::errc{} || ptr != paramEnd) {
        response = "Invalid breakpoint position: " + param;
      } else {
        size_t instr = 0;
        state->setBreakpoint(state, position, &instr);
        response = "Breakpoint set at instruction " + std::to_string(instr);
      }
    } else if (command == "diagnose" || command == "d") {
      std::vector<ErrorCause> problems(10);
      const auto count = state->getDiagnostics(state)->potentialErrorCauses(
          state->getDiagnostics(state), problems.data(), problems.size());
      response = std::to_string(count) + " potential problems found";
    } else if (command.starts_with("get ") || command.starts_with("g ")) {
      const auto varName = command.substr(command.find(' ') + 1);
      Variable v;
      std::ostringstream oss;
      if (state->getClassicalVariable(state, varName.c_str(), &v) == ERROR) {
        oss << "Variable " << varName << " not found";
      } else if (v.type == VarBool) {
        oss << std::boolalpha << varName << " = " << v.value.boolValue;
      } else if (v.type == VarInt) {
        oss << varName << " = " << v.value.intValue;
      } else if (v.type == VarFloat) {
        oss << varName << " = " << v.value.floatValue;
      }
      response = oss.str();
    } else if (command == "inspect" || command == "i") {
      inspecting = state->getCurrentInstruction(state);
    } else if (command == "reset" || command == "r") {
      state->resetSimulation(state);
    } else if (command == "state" || command == "s") {
      std::ostringstream oss;
      for (size_t i = 0; i < 1ULL << state->getNumQubits(state); i++) {
        Complex c;
        state->getAmplitudeIndex(state, i, &c);
        oss << c.real << " + " << c.imaginary << "i\n";
      }
      response = oss.str();
    } else {
      response = "Invalid command";
    }
  }
}

void CliFrontEnd::suggestUpdatedAssertions(SimulationState* state) {
  auto* diagnostics = state->getDiagnostics(state);
  std::string newCode = currentCode;
  const size_t count = 10;
  std::vector<std::array<char, 256>> newAssertions(count);
  std::vector<char*> newAssertionsPointers(count);
  std::ranges::transform(newAssertions, newAssertionsPointers.begin(),
                         [](std::array<char, 256>& arr) { return arr.data(); });
  std::vector<size_t> newPositions(count);

  size_t found = diagnostics->suggestNewAssertions(
      diagnostics, newPositions.data(), newAssertionsPointers.data(), count);
  std::set<size_t> coveredPositions;
  for (size_t i = found - 1; i != -1ULL; i--) {
    size_t start = 0;
    size_t end = 0;
    state->getInstructionPosition(state, newPositions[i], &start, &end);
    if (!coveredPositions.contains(newPositions[i])) {
      coveredPositions.insert(newPositions[i]);
      newCode.erase(start, end - start + 1);
    }
    newCode.insert(start, newAssertions[i].data());
  }

  state->resetSimulation(state);
  state->loadCode(state, newCode.c_str());
  size_t errors = 0;
  state->runAll(state, &errors);

  std::vector<size_t> beforeMove(count);
  std::vector<size_t> afterMove(count);
  diagnostics = state->getDiagnostics(state);
  found = diagnostics->suggestAssertionMovements(diagnostics, beforeMove.data(),
                                                 afterMove.data(), count);

  for (size_t i = 0; i < found; i++) {
    size_t start = 0;
    size_t end = 0;
    state->getInstructionPosition(state, beforeMove[i], &start, &end);
    if (end < newCode.length() - 1 &&
        (newCode[end + 1] == '\n' || newCode[end + 1] == ' ')) {
      end++;
    }
    const std::string assertion = newCode.substr(start, end - start + 1);
    newCode.erase(start, end - start + 1);
    state->getInstructionPosition(state, afterMove[i], &start, &end);
    newCode.insert(start, assertion);
    for (size_t j = i + 1; j < found; j++) {
      if (beforeMove[j] > beforeMove[i]) {
        beforeMove[j]--;
      }
      if (beforeMove[j] > afterMove[i]) {
        beforeMove[j]++;
      }
      if (afterMove[j] > afterMove[i]) {
        afterMove[j]++;
      }
      if (afterMove[j] > beforeMove[i]) {
        afterMove[j]--;
      }
    }
    state->loadCode(state, newCode.c_str());
  }

  std::cout << "Code with updated assertions is:\n";
  std::cout << "------------------------------------------------------------\n";
  std::cout << newCode << "\n";
  std::cout << "------------------------------------------------------------\n";

  state->resetSimulation(state);

  const LineEditor confirmEditor{std::cin, std::cout, "Accept? [y/n]: "};
  const auto reply = confirmEditor.readLine();
  const std::string command = reply.value_or("");

  if (command == "y") {
    currentCode = newCode;
  } else {
    state->loadCode(state, currentCode.c_str());
  }
}

void CliFrontEnd::printState(SimulationState* state, size_t inspecting,
                             bool codeOnly) {
  std::vector<size_t> highlightIntervals;
  if (inspecting != -1ULL) {
    std::vector<uint8_t> inspectingDependencies(
        state->getInstructionCount(state));
    auto* deps = inspectingDependencies.data();
    // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
    state->getDiagnostics(state)->getDataDependencies(
        state->getDiagnostics(state), inspecting, true,
        reinterpret_cast<bool*>(deps));
    // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
    uint8_t on = 0;
    for (size_t i = 0; i < inspectingDependencies.size(); i++) {
      if (inspectingDependencies[i] != on) {
        on = inspectingDependencies[i];
        size_t start = 0;
        size_t end = 0;
        state->getInstructionPosition(state, i, &start, &end);
        highlightIntervals.push_back(start);
      }
    }
  }
  if (highlightIntervals.empty()) {
    highlightIntervals.push_back(0);
  }
  highlightIntervals.push_back(currentCode.length() + 1);
  size_t currentStart = 0;
  size_t currentEnd = 0;
  const Result res = state->getInstructionPosition(
      state, state->getCurrentInstruction(state), &currentStart, &currentEnd);

  size_t currentPos = 0;
  bool on = false;
  std::ostringstream code;
  for (const auto nextInterval : highlightIntervals) {
    const auto* const textColor = on ? ANSI_BG_RESET : ANSI_COL_GRAY;
    if (res == OK && currentStart >= currentPos &&
        currentStart < nextInterval) {
      code << textColor
           << currentCode.substr(currentPos, currentStart - currentPos)
           << ANSI_BG_RESET;
      code << ANSI_BG_YELLOW
           << currentCode.substr(currentStart, currentEnd - currentStart + 1)
           << ANSI_BG_RESET;
      code << textColor
           << currentCode.substr(currentEnd + 1, nextInterval - currentEnd - 1)
           << ANSI_BG_RESET;
    } else {
      code << textColor
           << currentCode.substr(currentPos, nextInterval - currentPos)
           << ANSI_BG_RESET;
    }
    on = !on;
    currentPos = nextInterval;
  }
  std::cout << addLineNumbers(code.str());

  if (!codeOnly) {
    const auto bitStrings = getBitStrings(state->getNumQubits(state));
    Complex c;
    for (const auto& bitString : bitStrings) {
      state->getAmplitudeBitstring(state, bitString.c_str(), &c);
      std::cout << bitString << " " << c.real << "\t||\t";
    }
    std::cout << "\n";
  }
  if (state->didAssertionFail(state)) {
    std::cout << "THIS LINE FAILED AN ASSERTION\n";
  }
}

} // namespace mqt::debugger
