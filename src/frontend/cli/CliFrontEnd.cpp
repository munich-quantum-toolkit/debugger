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
#include <format>
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

/// Largest qubit count for which the `state` command dumps every amplitude.
constexpr size_t MAX_STATE_VIEW_QUBITS = 6;

/// Largest qubit count for which the on-screen amplitudes table is rendered.
constexpr size_t MAX_AMPLITUDES_VIEW_QUBITS = 3;

size_t boundedStrnlen(const char* data, size_t max) {
  const auto* end = static_cast<const char*>(std::memchr(data, '\0', max));
  return end != nullptr ? static_cast<size_t>(end - data) : max;
}

std::string_view loadResultMessageView(const LoadResult& result) {
  const auto* data = std::data(result.message);
  return {data, boundedStrnlen(data, LOAD_RESULT_MESSAGE_MAX)};
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
      bitString.insert(bitString.begin(), (i & (1ULL << j)) > 0 ? '1' : '0');
    }
    bitStrings.push_back(bitString);
  }
  return bitStrings;
}

} // namespace

CliFrontEnd::CliFrontEnd(std::ostream& out) : renderer(out) {}

void CliFrontEnd::initCode(const char* code) {
  currentCode = code;
  // Trim trailing newlines so downstream consumers can rely on the invariant.
  while (!currentCode.empty() && currentCode.back() == '\n') {
    currentCode.pop_back();
  }
}

void CliFrontEnd::run(const char* code, SimulationState* state) {
  initCode(code);

  const auto result = state->loadCode(state, code);
  state->resetSimulation(state);
  if (result.status != LOAD_OK) {
    const auto messageView = loadResultMessageView(result);
    renderer.println(std::format(
        "Error loading code{}",
        messageView.empty() ? "" : std::format(": {}", messageView)));
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
  std::optional<size_t> inspecting;

  while (command != "q" && command != "quit") {
    printScreen(state, inspecting, response);
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
    } else if (command == "a" || command == "assertions") {
      suggestUpdatedAssertions(state);
    } else if (command.starts_with("b ") ||
               command.starts_with("breakpoint ")) {
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
        size_t start = 0;
        size_t end = 0;
        if (state->setBreakpoint(state, *offset, &instr) != OK ||
            state->getInstructionPosition(state, instr, &start, &end) != OK) {
          response = "Could not set breakpoint at line " + param;
        } else {
          const auto startLine = charOffsetToLine(currentCode, start);
          const auto endLine = charOffsetToLine(currentCode, end);
          for (auto l = startLine; l <= endLine; ++l) {
            breakpointLines.insert(l);
          }
          response = "Breakpoint set at " +
                     (startLine == endLine
                          ? std::format("line {}", startLine)
                          : std::format("lines {}-{}", startLine, endLine));
        }
      }
    } else if (command == "d" || command == "diagnose") {
      std::vector<ErrorCause> problems(10);
      const auto count = state->getDiagnostics(state)->potentialErrorCauses(
          state->getDiagnostics(state), problems.data(), problems.size());
      response = std::to_string(count) + " potential problems found";
    } else if (command.starts_with("g ") || command.starts_with("get ")) {
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
    } else if (command == "i" || command == "inspect") {
      const auto current = state->getCurrentInstruction(state);
      inspecting = current;
      std::vector<uint8_t> deps(state->getInstructionCount(state));
      // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
      state->getDiagnostics(state)->getDataDependencies(
          state->getDiagnostics(state), current, true,
          reinterpret_cast<bool*>(deps.data()));
      // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
      if (std::ranges::count(deps, uint8_t{1}) == 1) {
        response = "Current instruction has no data dependencies";
      }
    } else if (command == "r" || command == "reset") {
      state->resetSimulation(state);
      inspecting.reset();
    } else if (command == "s" || command == "state") {
      if (state->getNumQubits(state) > MAX_STATE_VIEW_QUBITS) {
        response = std::format("The state command supports up to {} qubits",
                               MAX_STATE_VIEW_QUBITS);
      } else {
        const auto bitStrings = getBitStrings(state->getNumQubits(state));
        std::vector<std::string> lines;
        lines.reserve(bitStrings.size());
        for (size_t i = 0; i < bitStrings.size(); i++) {
          Complex c;
          state->getAmplitudeIndex(state, i, &c);
          lines.push_back(std::format("{}: {:.6g} + {:.6g}i", bitStrings[i],
                                      c.real, c.imaginary));
        }
        response = join(lines, "\n");
      }
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

void CliFrontEnd::printScreen(SimulationState* state,
                              std::optional<size_t> inspecting,
                              std::string_view response) {
  renderer.clearScreen();
  printHelpBar();
  printCode(state, inspecting);
  printAmplitudes(state);
  if (state->didAssertionFail(state)) {
    renderer.println("THIS LINE FAILED AN ASSERTION");
  }
  if (!response.empty()) {
    renderer.println(response);
  }
}

void CliFrontEnd::printCode(SimulationState* state,
                            std::optional<size_t> inspecting) {
  // 1-based line numbers whose instructions are data dependencies of the
  // inspected instruction. Empty when not inspecting.
  std::set<size_t> depLines;
  if (inspecting.has_value()) {
    std::vector<uint8_t> deps(state->getInstructionCount(state));
    // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
    state->getDiagnostics(state)->getDataDependencies(
        state->getDiagnostics(state), *inspecting, true,
        reinterpret_cast<bool*>(deps.data()));
    // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
    for (size_t i = 0; i < deps.size(); ++i) {
      // Skip self: the current instruction already has its own highlight;
      // marking it as its own dependency would be redundant.
      if (deps[i] == 0 || i == *inspecting) {
        continue;
      }
      size_t start = 0;
      size_t end = 0;
      state->getInstructionPosition(state, i, &start, &end);
      const auto startLine = charOffsetToLine(currentCode, start);
      const auto endLine = charOffsetToLine(currentCode, end);
      for (auto l = startLine; l <= endLine; ++l) {
        depLines.insert(l);
      }
    }
  }

  size_t curStart = 0;
  size_t curEnd = 0;
  const bool hasCurrent =
      state->getInstructionPosition(state, state->getCurrentInstruction(state),
                                    &curStart, &curEnd) == OK;

  // Each rendered line closes its own ANSI escapes before its newline, so
  // terminal styling never carries across into the next line.
  auto lines = currentCode | std::views::split('\n');
  const auto lineCount = static_cast<size_t>(std::ranges::distance(lines));
  const auto gutterWidth = std::to_string(lineCount).size();

  size_t lineNum = 1;
  size_t lineStart = 0;
  for (const auto& lineView : lines) {
    const std::string_view line{lineView.begin(), lineView.end()};
    const size_t lineEnd = lineStart + line.size();
    const bool isDep = depLines.contains(lineNum);
    const bool containsCurrent =
        hasCurrent && curStart <= lineEnd && curEnd >= lineStart;
    const auto codeStyle = [isDep](std::string_view text) {
      return isDep ? bold(fgColor(text, ansi::FG_WHITE)) : std::string{text};
    };
    const auto hlStyle = [](std::string_view text) {
      return bgColor(fgColor(text, ansi::FG_CODE_HL), ansi::BG_CODE_HL);
    };

    auto gutter = rightAlign(std::to_string(lineNum), gutterWidth);
    if (isDep) {
      gutter = bold(fgColor(gutter, ansi::FG_WHITE));
    }
    if (breakpointLines.contains(lineNum)) {
      gutter = bgColor(gutter, ansi::BG_BREAKPOINT);
    }

    std::string codeStr;
    if (containsCurrent) {
      const size_t hlBegin = curStart > lineStart ? curStart - lineStart : 0;
      const size_t hlEnd = std::min(curEnd - lineStart + 1, line.size());
      if (hlBegin > 0) {
        codeStr += codeStyle(line.substr(0, hlBegin));
      }
      if (hlEnd > hlBegin) {
        codeStr += hlStyle(line.substr(hlBegin, hlEnd - hlBegin));
      }
      if (hlEnd < line.size()) {
        codeStr += codeStyle(line.substr(hlEnd));
      }
    } else {
      codeStr = codeStyle(line);
    }

    renderer.println(std::format("{} {}", gutter, codeStr));
    ++lineNum;
    lineStart = lineEnd + 1;
  }
}

void CliFrontEnd::printAmplitudes(SimulationState* state) {
  // Check qubit count.
  if (state->getNumQubits(state) > MAX_AMPLITUDES_VIEW_QUBITS) {
    renderer.println(std::format(
        "Amplitudes table hidden for more than {} qubits. "
        "Use the 'state' command instead.",
        MAX_AMPLITUDES_VIEW_QUBITS));
    return;
  }

  const auto bitStrings = getBitStrings(state->getNumQubits(state));

  // Build vector of amplitude strings.
  std::vector<std::string> amplitudes;
  amplitudes.reserve(bitStrings.size());
  for (const auto& bitString : bitStrings) {
    Complex c;
    state->getAmplitudeBitstring(state, bitString.c_str(), &c);
    amplitudes.push_back(std::format("{:.6g} + {:.6g}i", c.real, c.imaginary));
  }

  // Build vector of column widths.
  const size_t nCols = bitStrings.size();
  std::vector<size_t> widths(nCols);
  for (size_t i = 0; i < nCols; ++i) {
    widths[i] = std::max(bitStrings[i].size(), amplitudes[i].size());
  }

  // Render header.
  renderer.println(bgColor(fgColor(margins(bold("Amplitudes")), ansi::FG_BLACK),
                           ansi::BG_TABLE_HEADER));

  // Render first row.
  std::vector<std::string> bsCells(nCols);
  for (size_t i = 0; i < nCols; ++i) {
    bsCells[i] = bold(margins(rightAlign(bitStrings[i], widths[i])));
  }
  renderer.println(bgColor(fgColor(join(bsCells, "|"), ansi::FG_BLACK),
                           ansi::BG_TABLE_TOP_ROW));

  // Render second row.
  std::vector<std::string> ampCells(nCols);
  for (size_t i = 0; i < nCols; ++i) {
    ampCells[i] = margins(rightAlign(amplitudes[i], widths[i]));
  }
  renderer.println(bgColor(fgColor(join(ampCells, "|"), ansi::FG_WHITE),
                           ansi::BG_TABLE_BOTTOM_ROW));
}

} // namespace mqt::debugger
