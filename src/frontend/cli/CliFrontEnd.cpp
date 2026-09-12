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
#include <iostream>
#include <iterator>
#include <memory>
#include <numeric>
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

// Style modifiers.
constexpr std::string_view ANSI_RESET = "\x1b[0m";
constexpr std::string_view ANSI_BOLD = "\x1b[1m";
constexpr std::string_view ANSI_NORMAL = "\x1b[22m";

// Foreground colors.
constexpr std::string_view ANSI_FG_BLACK = "\x1b[30m";
constexpr std::string_view ANSI_FG_WHITE = "\x1b[97m";
constexpr std::string_view ANSI_FG_CODE_DIM = "\x1b[90m";
constexpr std::string_view ANSI_FG_CODE_HL = ANSI_FG_BLACK;

// Background colors.
constexpr std::string_view ANSI_BG_BREAKPOINT = "\x1b[41m";
constexpr std::string_view ANSI_BG_CODE_HL = "\x1b[48;5;227m";
constexpr std::string_view ANSI_BG_TABLE_HEADER = "\x1b[47m";
constexpr std::string_view ANSI_BG_TABLE_ROW_EVEN = "\x1b[48;5;153m";
constexpr std::string_view ANSI_BG_TABLE_ROW_ODD = "\x1b[44m";

/**
 * @brief Wrap @p content between one space on each side.
 */
std::string margins(std::string_view content) {
  std::string s = " ";
  s.append(content);
  s += ' ';
  return s;
}

/**
 * @brief Right-pad @p content with spaces up to @p width. Left-aligned.
 */
std::string leftAlign(std::string_view content, size_t width) {
  std::string s{content};
  if (content.size() < width) {
    s.append(width - content.size(), ' ');
  }
  return s;
}

/**
 * @brief Left-pad @p content with spaces up to @p width. Right-aligned.
 */
std::string rightAlign(std::string_view content, size_t width) {
  const auto pad = content.size() < width ? width - content.size() : 0;
  std::string s(pad, ' ');
  s.append(content);
  return s;
}

/**
 * @brief Wrap @p content between bold on and bold off codes.
 */
std::string bold(std::string_view content) {
  std::string s{ANSI_BOLD};
  s.append(content);
  s.append(ANSI_NORMAL);
  return s;
}

/**
 * @brief Wrap @p content between background color @p bg and a full reset.
 */
std::string bgColor(std::string_view content, std::string_view bg) {
  std::string s{bg};
  s.append(content);
  s.append(ANSI_RESET);
  return s;
}

/**
 * @brief Wrap @p content between foreground color @p fg and a full reset.
 */
std::string fgColor(std::string_view content, std::string_view fg) {
  std::string s{fg};
  s.append(content);
  s.append(ANSI_RESET);
  return s;
}

/**
 * @brief Return the widest cell in column @p col of @p rows.
 */
size_t colMaxWidth(const std::vector<std::vector<std::string>>& rows,
                   size_t col) {
  return std::ranges::max(rows | std::views::transform([col](const auto& row) {
                            return row[col].size();
                          }));
}

/**
 * @brief Join @p parts with @p sep between adjacent parts.
 *
 * @param parts The strings to join.
 * @param sep The separator to place between consecutive parts.
 * @return The joined string, or an empty string if @p parts is empty.
 */
std::string join(const std::vector<std::string>& parts, std::string_view sep) {
  if (parts.empty()) {
    return {};
  }
  return std::accumulate(std::next(parts.begin()), parts.end(),
                         std::string{parts.front()},
                         [sep](std::string acc, std::string_view p) {
                           acc.append(sep);
                           acc.append(p);
                           return acc;
                         });
}

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
      gutter = bgColor(gutter, ANSI_BG_BREAKPOINT);
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

/**
 * @brief Print a two-part table: a header cell on the top-left and rows of
 * data cells to its right.
 *
 * The (row 0, col 0) cell shows @p header on a white background in bold; the
 * corresponding column-0 cells of the remaining rows are painted black as a
 * visual divider. Data rows alternate their background color: even rows go
 * on light blue and have their contents in bold, odd rows go on dark blue in
 * normal weight. Each data column is padded to the widest cell across all
 * rows.
 *
 * @param header Text placed in the header cell.
 * @param rows Data rows. All rows must have the same number of cells.
 */
void printTable(std::string_view header,
                const std::vector<std::vector<std::string>>& rows) {
  if (rows.empty()) {
    return;
  }

  // Column widths: max cell size across all rows per column.
  const auto nCols = rows[0].size();
  std::vector<size_t> widths(nCols);
  std::ranges::transform(
      std::views::iota(size_t{0}, nCols), widths.begin(),
      [&rows](size_t col) { return colMaxWidth(rows, col); });

  // Header row.
  std::cout << bgColor(fgColor(margins(bold(header)), ANSI_FG_BLACK),
                       ANSI_BG_TABLE_HEADER)
            << "\n";

  // Data rows: alternate light/dark background, bold on even rows.
  for (size_t r = 0; r < rows.size(); ++r) {
    const bool even = (r % 2 == 0);
    std::vector<std::string> cells(nCols);
    std::ranges::transform(rows[r], widths, cells.begin(),
                           [even](std::string_view content, size_t width) {
                             const auto cell =
                                 margins(leftAlign(content, width));
                             return even ? bold(cell) : cell;
                           });
    const auto rowBg = even ? ANSI_BG_TABLE_ROW_EVEN : ANSI_BG_TABLE_ROW_ODD;
    const auto rowFg = even ? ANSI_FG_BLACK : ANSI_FG_WHITE;
    std::cout << bgColor(fgColor(join(cells, "|"), rowFg), rowBg) << "\n";
  }
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

void CliFrontEnd::printHelpBar() {
  printTable(
      "MQT Debugger",
      {
          {"F5", "F6", "F7", "F9", "F10", "F11", "q"},
          {"Run", "Step", "Step over", "Run back", "Back", "Back over", "Quit"},
          {"a", "b     <N>", "d", "g   <var>", "i", "r", "s"},
          {"Assertions", "Break <N>", "Diagnose", "Get <var>", "Inspect",
           "Reset", "State"},
      });
}

void CliFrontEnd::printScreen(SimulationState* state, size_t inspecting,
                              std::string_view response, bool codeOnly) {
  clearScreen();
  printHelpBar();
  printCode(state, inspecting);
  if (!codeOnly) {
    printAmplitudes(state);
  }
  if (state->didAssertionFail(state)) {
    std::cout << "THIS LINE FAILED AN ASSERTION\n";
  }
  if (!response.empty()) {
    std::cout << response << "\n";
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
    return fgColor(text, ANSI_FG_CODE_DIM);
  };
  const auto hlCode = [](std::string_view text) {
    return bgColor(fgColor(text, ANSI_FG_CODE_HL), ANSI_BG_CODE_HL);
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
  std::cout << addLineNumbers(code.str(), breakpointLines);
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

  printTable("Amplitudes", {bitStrings, amplitudes});
}

} // namespace mqt::debugger
