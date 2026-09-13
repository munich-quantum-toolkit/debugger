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
#include "frontend/cli/Renderer.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <iterator>
#include <memory>
#include <optional>
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
 * @brief Prefix each source line in @p text with a right-aligned 1-based line
 * number.
 *
 * The gutter width is picked from the total line count so all separators line
 * up.
 *
 * Preserves any ANSI color codes already present in @p text.
 *
 * Only newline characters trigger a new gutter, so highlights that span
 * newlines still render correctly.
 *
 * Line numbers whose 1-based index is in @p breakpointLines are painted with a
 * red background so the user sees at a glance where the active breakpoints are.
 *
 * @param text The source text to number.
 * @param breakpointLines 1-based line numbers whose gutter number should be
 * highlighted.
 * @return The numbered text ready to send to stdout.
 */
std::string addLineNumbers(std::string_view text,
                           const std::set<size_t>& breakpointLines) {
  if (text.empty()) {
    return {};
  }

  auto lines = text | std::views::split('\n');
  const auto lineCount = static_cast<size_t>(std::ranges::distance(lines));
  const auto gutterWidth = std::to_string(lineCount).size();

  std::ostringstream oss;
  size_t lineNum = 1;
  for (const auto& line : lines) {
    const std::string_view code{line.begin(), line.end()};
    auto gutter = rightAlign(std::to_string(lineNum), gutterWidth);
    if (breakpointLines.contains(lineNum)) {
      gutter = bgColor(gutter, ansi::BG_BREAKPOINT);
    }
    oss << gutter << ' ' << code << "\n";
    ++lineNum;
  }
  return oss.str();
}

/**
 * @brief Return the character offset of the start of a given line in @p code.
 *
 * Used to translate the user-facing line number of `breakpoint <N>` into the
 * character-offset position that the simulator's `setBreakpoint` API expects.
 *
 * @param code The source code to scan.
 * @param lineNumber 1-based line number to look up.
 * @return The 0-based character offset of the start of the requested line.
 * Or `std::nullopt` if the line number is out of range.
 */
std::optional<size_t> lineToCharOffset(std::string_view code,
                                       size_t lineNumber) {
  auto lines = code | std::views::split('\n');
  const auto lineCount = static_cast<size_t>(std::ranges::distance(lines));
  if (lineNumber == 0 || lineNumber > lineCount) {
    return std::nullopt;
  }
  size_t offset = 0;
  for (const auto& line : lines | std::views::take(lineNumber - 1)) {
    offset += static_cast<size_t>(std::ranges::distance(line)) + 1;
  }
  return offset;
}

/**
 * @brief Return the 1-based line number that contains @p offset in @p code.
 *
 * Used to translate the character offset the simulator reports for a breakpoint
 * back into a user-facing line number. Inverse of `lineToCharOffset`.
 *
 * @param code The source code to scan.
 * @param offset A 0-based character offset into @p code.
 * @return The 1-based line number containing @p offset.
 */
size_t charOffsetToLine(std::string_view code, size_t offset) {
  const auto prefix = code.substr(0, std::min(offset, code.size()));
  return static_cast<size_t>(std::ranges::count(prefix, '\n')) + 1;
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

CliFrontEnd::CliFrontEnd(std::ostream& out) : renderer(out) {}

void CliFrontEnd::initCode(const char* code) { currentCode = code; }

void CliFrontEnd::run(const char* code, SimulationState* state) {
  initCode(code);

  const auto result = state->loadCode(state, code);
  state->resetSimulation(state);
  if (result.status != LOAD_OK) {
    const auto messageView = loadResultMessageView(result);
    if (!messageView.empty()) {
      renderer.print("Error loading code: ");
      renderer.println(messageView);
    } else {
      renderer.println("Error loading code");
    }
    return;
  }

  const RawModeTerminal rawMode;
  LineEditor editor{std::cin, std::cout, "$ "};
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
    printScreen(state, inspecting, response, state->getNumQubits(state) >= 7);
    // The editor is printing the prompt before reading the line
    auto line = editor.readLine();
    if (!line.has_value()) {
      break;
    }
    command = std::move(*line);
    const bool wasFKey = editor.wasBound();
    if (!command.empty() && !wasFKey) {
      editor.addToHistory(command);
    }
    response.clear();
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
      size_t lineNumber = 0;
      const auto [ptr, ec] = std::from_chars(paramBegin, paramEnd, lineNumber);
      if (ec != std::errc{} || ptr != paramEnd) {
        response = "Invalid breakpoint line: " + param;
      } else if (const auto offset = lineToCharOffset(currentCode, lineNumber);
                 !offset.has_value()) {
        response = "Line number out of range: " + param;
      } else {
        size_t instr = 0;
        state->setBreakpoint(state, *offset, &instr);
        size_t start = 0;
        size_t end = 0;
        state->getInstructionPosition(state, instr, &start, &end);
        const auto bpLine = charOffsetToLine(currentCode, start);
        breakpointLines.insert(bpLine);
        response = "Breakpoint set at line " + std::to_string(bpLine);
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

  renderer.println("Code with updated assertions is:");
  renderer.println(
      "------------------------------------------------------------");
  renderer.println(newCode);
  renderer.println(
      "------------------------------------------------------------");

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

void CliFrontEnd::printHelpBar() {
  using Cell = std::pair<std::string_view, std::string_view>;
  const std::vector<Cell> row1 = {{"F5", "Run"},       {"F6", "Step"},
                                  {"F7", "Step over"}, {"F9", "Run back"},
                                  {"F10", "Back"},     {"F11", "Back over"},
                                  {"q", "Quit"}};
  const std::vector<Cell> row2 = {{"a", "Assertions"}, {"b <line#>", "Break"},
                                  {"d", "Diagnose"},   {"g <var>", "Get"},
                                  {"i", "Inspect"},    {"r", "Reset"},
                                  {"s", "State"}};

  const size_t n = row1.size();
  std::vector<size_t> keyWidths(n);
  std::vector<size_t> textWidths(n);
  for (size_t i = 0; i < n; ++i) {
    keyWidths[i] = std::max(row1[i].first.size(), row2[i].first.size());
    textWidths[i] = std::max(row1[i].second.size(), row2[i].second.size());
  }

  const auto renderRow = [&](const std::vector<Cell>& row) {
    std::vector<std::string> cells(row.size());
    for (size_t i = 0; i < row.size(); ++i) {
      const auto& [key, desc] = row[i];
      cells[i] = bold(rightAlign(key, keyWidths[i])) + " " +
                 leftAlign(desc, textWidths[i]);
    }
    return join(cells, " | ");
  };

  renderer.println(
      bgColor(fgColor(margins(bold("MQT Debugger")), ansi::FG_BLACK),
              ansi::BG_TABLE_HEADER));
  renderer.println(bgColor(fgColor(margins(renderRow(row1)), ansi::FG_BLACK),
                           ansi::BG_TABLE_TOP_ROW));
  renderer.println(bgColor(fgColor(margins(renderRow(row2)), ansi::FG_WHITE),
                           ansi::BG_TABLE_BOTTOM_ROW));
}

void CliFrontEnd::printScreen(SimulationState* state, size_t inspecting,
                              std::string_view response, bool codeOnly) {
  renderer.clearScreen();
  printHelpBar();
  printCode(state, inspecting);
  if (!codeOnly) {
    printAmplitudes(state);
  }
  if (state->didAssertionFail(state)) {
    renderer.println("THIS LINE FAILED AN ASSERTION");
  }
  if (!response.empty()) {
    renderer.println(response);
  }
}

void CliFrontEnd::printCode(SimulationState* state, size_t inspecting) {
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
  // Ignore trailing newlines when picking the sentinel: they would be copied
  // into the printed code and later show up as empty numbered lines.
  size_t trimmedLength = currentCode.size();
  while (trimmedLength > 0 && currentCode[trimmedLength - 1] == '\n') {
    --trimmedLength;
  }
  highlightIntervals.push_back(trimmedLength + 1);
  size_t currentStart = 0;
  size_t currentEnd = 0;
  const Result res = state->getInstructionPosition(
      state, state->getCurrentInstruction(state), &currentStart, &currentEnd);

  size_t currentPos = 0;
  bool on = false;
  std::ostringstream code;
  const auto plainCode = [](std::string_view text) {
    return std::string{text};
  };
  const auto dimCode = [](std::string_view text) {
    return fgColor(text, ansi::FG_CODE_DIM);
  };
  const auto hlCode = [](std::string_view text) {
    return bgColor(fgColor(text, ansi::FG_CODE_HL), ansi::BG_CODE_HL);
  };
  for (const auto nextInterval : highlightIntervals) {
    const auto nonHlCode = on ? plainCode : dimCode;
    const bool containsHighlight =
        currentStart >= currentPos && currentStart < nextInterval;
    if (res == OK && containsHighlight) {
      const auto preHl =
          currentCode.substr(currentPos, currentStart - currentPos);
      const auto hl =
          currentCode.substr(currentStart, currentEnd - currentStart + 1);
      const auto postHl =
          currentCode.substr(currentEnd + 1, nextInterval - currentEnd - 1);
      code << nonHlCode(preHl) << hlCode(hl) << nonHlCode(postHl);
    } else {
      const auto section =
          currentCode.substr(currentPos, nextInterval - currentPos);
      code << nonHlCode(section);
    }
    on = !on;
    currentPos = nextInterval;
  }
  renderer.print(addLineNumbers(code.str(), breakpointLines));
}

void CliFrontEnd::printAmplitudes(SimulationState* state) {
  const auto bitStrings = getBitStrings(state->getNumQubits(state));

  std::vector<std::string> amplitudes;
  amplitudes.reserve(bitStrings.size());
  for (const auto& bitString : bitStrings) {
    Complex c;
    state->getAmplitudeBitstring(state, bitString.c_str(), &c);
    std::ostringstream oss;
    oss << c.real;
    amplitudes.push_back(oss.str());
  }

  const size_t nCols = bitStrings.size();
  std::vector<size_t> widths(nCols);
  for (size_t i = 0; i < nCols; ++i) {
    widths[i] = std::max(bitStrings[i].size(), amplitudes[i].size());
  }

  renderer.println(bgColor(fgColor(margins(bold("Amplitudes")), ansi::FG_BLACK),
                           ansi::BG_TABLE_HEADER));

  std::vector<std::string> bsCells(nCols);
  for (size_t i = 0; i < nCols; ++i) {
    bsCells[i] = bold(margins(rightAlign(bitStrings[i], widths[i])));
  }
  renderer.println(bgColor(fgColor(join(bsCells, "|"), ansi::FG_BLACK),
                           ansi::BG_TABLE_TOP_ROW));

  std::vector<std::string> ampCells(nCols);
  for (size_t i = 0; i < nCols; ++i) {
    ampCells[i] = margins(rightAlign(amplitudes[i], widths[i]));
  }
  renderer.println(bgColor(fgColor(join(ampCells, "|"), ansi::FG_WHITE),
                           ansi::BG_TABLE_BOTTOM_ROW));
}

} // namespace mqt::debugger
