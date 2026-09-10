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
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <set>
#include <string>
#include <string_view>
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

  std::string command;
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

  bool wasError = false;
  bool wasGet = false;
  size_t inspecting = -1ULL;

  const RawModeTerminal rawMode;
  LineEditor editor{std::cin, std::cout, "Enter command: "};
  editor.bindKey("15~", "run");       // F5
  editor.bindKey("17~", "step");      // F6
  editor.bindKey("18~", "step over"); // F7
  editor.bindKey("20~", "run back");  // F9
  editor.bindKey("21~", "back");      // F10
  editor.bindKey("23~", "back over"); // F11

  while (command != "quit" && command != "q") {
    clearScreen();
    if (wasError) {
      std::cout << "Invalid command. Choose one of:\n";
      std::cout << "run [F5]\t";
      std::cout << "step [F6 | Enter]\t";
      std::cout << "step over [F7]\t";
      std::cout << "run back [F9]\t";
      std::cout << "back [F10]\t";
      std::cout << "back over [F11]\t";
      std::cout << "assertions [a]\t";
      std::cout << "breakpoint <N> [b <N>]\t";
      std::cout << "diagnose [d]\t";
      std::cout << "get <variable> [g <variable>]\t";
      std::cout << "inspect [i]\t";
      std::cout << "reset [r]\t";
      std::cout << "state [s]\t";
      std::cout << "quit [q]\n\n";
      wasError = false;
    }
    if (wasGet) {
      const auto varName = command.substr(command.find(' ') + 1);
      Variable v;
      if (state->getClassicalVariable(state, varName.c_str(), &v) == ERROR) {
        std::cout << "Variable " << varName << " not found\n";
      } else {
        if (v.type == VarBool) {
          std::cout << varName << " = "
                    << (v.value.boolValue ? "true" : "false") << "\n";
        } else if (v.type == VarInt) {
          std::cout << varName << " = " << v.value.intValue << "\n";
        } else if (v.type == VarFloat) {
          std::cout << varName << " = " << v.value.floatValue << "\n";
        }
      }
      wasGet = false;
    }
    printState(state, inspecting, state->getNumQubits(state) >= 7);

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
      const auto position = std::stoul(param);
      size_t instr = 0;
      state->setBreakpoint(state, position, &instr);
      std::cout << "Breakpoint set at instruction " << instr << "\n";
    } else if (command == "diagnose" || command == "d") {
      std::vector<ErrorCause> problems(10);
      const auto count = state->getDiagnostics(state)->potentialErrorCauses(
          state->getDiagnostics(state), problems.data(), problems.size());
      std::cout << count << " potential problems found\n";
    } else if (command.starts_with("get ") || command.starts_with("g ")) {
      wasGet = true;
    } else if (command == "inspect" || command == "i") {
      inspecting = state->getCurrentInstruction(state);
    } else if (command == "reset" || command == "r") {
      state->resetSimulation(state);
    } else if (command == "state" || command == "s") {
      for (size_t i = 0; i < 1ULL << state->getNumQubits(state); i++) {
        Complex c;
        state->getAmplitudeIndex(state, i, &c);
        std::cout << c.real << " + " << c.imaginary << "i\n";
      }
      LineEditor{std::cin, std::cout, "Press Enter to continue: "}.readLine();
    } else {
      wasError = true;
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

  LineEditor confirmEditor{std::cin, std::cout, "Accept? [y/n]: "};
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
  for (const auto nextInterval : highlightIntervals) {
    const auto* const textColor = on ? ANSI_BG_RESET : ANSI_COL_GRAY;
    if (res == OK && currentStart >= currentPos &&
        currentStart < nextInterval) {
      std::cout << textColor
                << currentCode.substr(currentPos, currentStart - currentPos)
                << ANSI_BG_RESET;
      std::cout << ANSI_BG_YELLOW
                << currentCode.substr(currentStart,
                                      currentEnd - currentStart + 1)
                << ANSI_BG_RESET;
      std::cout << textColor
                << currentCode.substr(currentEnd + 1,
                                      nextInterval - currentEnd - 1)
                << ANSI_BG_RESET;
    } else {
      std::cout << textColor
                << currentCode.substr(currentPos, nextInterval - currentPos)
                << ANSI_BG_RESET;
    }
    on = !on;
    currentPos = nextInterval;
  }
  std::cout << "\n";

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
