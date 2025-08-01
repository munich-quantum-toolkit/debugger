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
 * @file CliFrontEnd.cpp
 * @brief Implementation of the command-line interface front end.
 */
#include "frontend/cli/CliFrontEnd.hpp"

#include "backend/debug.h"
#include "backend/diagnostics.h"
#include "common.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <set>
#include <string>
#include <vector>

namespace mqt::debugger {

namespace {

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
  if (result == ERROR) {
    std::cout << "Error loading code\n";
    return;
  }

  bool wasError = false;
  bool wasGet = false;
  size_t inspecting = -1ULL;

  while (command != "exit") {
    clearScreen();
    if (wasError) {
      std::cout << "Invalid command. Choose one of:\n";
      std::cout << "run\t";
      std::cout << "run back [rb]\t";
      std::cout << "step [enter]\t";
      std::cout << "step over [o]\t";
      std::cout << "back [b]\t";
      std::cout << "back over [bo]\t";
      std::cout << "get <variable>\t";
      std::cout << "reset\t";
      std::cout << "inspect\t";
      std::cout << "assertions\t";
      std::cout << "exit\n\n";
      wasError = false;
    }
    if (wasGet) {
      Variable v;
      if (state->getClassicalVariable(
              state, command.substr(4, command.length() - 4).c_str(), &v) ==
          ERROR) {
        std::cout << "Variable " << command << " not found\n";
      } else {
        if (v.type == VarBool) {
          std::cout << command.substr(4, command.length() - 4) << " = "
                    << (v.value.boolValue ? "true" : "false") << "\n";
        } else if (v.type == VarInt) {
          std::cout << command.substr(4, command.length() - 4) << " = "
                    << v.value.intValue << "\n";
        } else if (v.type == VarFloat) {
          std::cout << command.substr(4, command.length() - 4) << " = "
                    << v.value.floatValue << "\n";
        }
      }
      wasGet = false;
    }
    printState(state, inspecting, state->getNumQubits(state) >= 7);

    std::cout << "Enter command: ";
    std::getline(std::cin, command);
    if (command == "run") {
      state->runSimulation(state);
    } else if (command == "run back" || command == "rb") {
      state->runSimulationBackward(state);
    } else if (command == "step" || command.empty()) {
      state->stepForward(state);
    } else if (command == "step over" || command == "o") {
      state->stepOverForward(state);
    } else if (command == "back" || command == "b") {
      state->stepBackward(state);
    } else if (command == "back over" || command == "bo") {
      state->stepOverBackward(state);
    } else if (command == "reset") {
      state->resetSimulation(state);
    } else if (command.starts_with("get ")) {
      wasGet = true;
    } else if (command == "inspect") {
      inspecting = state->getCurrentInstruction(state);
    } else if (command == "diagnose") {
      std::vector<ErrorCause> problems(10);
      const auto count = state->getDiagnostics(state)->potentialErrorCauses(
          state->getDiagnostics(state), problems.data(), problems.size());
      std::cout << count << " potential problems found\n";
    } else if (command.starts_with("breakpoint")) {
      const auto param = command.substr(11, command.length() - 11);
      const auto breakpoint = std::stoul(param);
      size_t instr = 0;
      state->setBreakpoint(state, breakpoint, &instr);
      std::cout << "Breakpoint set at instruction " << instr << "\n";
    } else if (command == "assertions") {
      suggestUpdatedAssertions(state);
    } else if (command == "state") {
      for (size_t i = 0; i < 1ULL << state->getNumQubits(state); i++) {
        Complex c;
        state->getAmplitudeIndex(state, i, &c);
        std::cout << c.real << " + " << c.imaginary << "i\n";
      }
      std::cin >> command;
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

  std::cout << "Accept? [y/n]: ";
  std::string command;
  std::getline(std::cin, command);

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
