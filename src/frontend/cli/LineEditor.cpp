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
 * @file LineEditor.cpp
 * @brief Implementation of the `LineEditor` CLI helper.
 */

#include "frontend/cli/LineEditor.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdio>
#include <istream>
#include <iterator>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>

namespace mqt::debugger {

namespace {

constexpr char CTRL_U = 0x15;
constexpr char BACKSPACE_DEL = 0x7f;
constexpr char BACKSPACE_BS = 0x08;
constexpr char ESC = 0x1b;
constexpr char CR = '\r';
constexpr char LF = '\n';

/// @brief Locale-agnostic whitespace check that avoids UB on signed `char`.
bool isSpace(char c) {
  return std::isspace(static_cast<unsigned char>(c)) != 0;
}

/**
 * @brief Read a CSI (Control Sequence Introducer) escape sequence after
 * `ESC` has been consumed.
 *
 * A CSI sequence has the form `ESC [ params final`, where `params` are the
 * parameter bytes (may be empty) and `final` is a letter or `~` that
 * identifies the command.
 *
 * @param in Input stream, positioned just after the `ESC` byte.
 * @param params Out parameter: parameter bytes of the sequence.
 * @param final Out parameter: final byte of the sequence.
 * @return true on a well-formed sequence; false if it is malformed or the
 * stream ends mid-sequence.
 */
bool readCsi(std::istream& in, std::string& params, char& final) {
  const int next = in.get();
  if (next != '[') {
    return false;
  }
  params.clear();
  while (true) {
    const int c = in.get();
    if (c == EOF) {
      return false;
    }
    const auto uc = static_cast<unsigned char>(c);
    if ((std::isalpha(uc) != 0) || uc == '~') {
      final = static_cast<char>(uc);
      return true;
    }
    params += static_cast<char>(uc);
  }
}

/**
 * @brief Return the index of the start of the previous word.
 *
 * Skips backwards over whitespace first, then over non-whitespace.
 *
 * @param buffer Input buffer to scan.
 * @param cursor Current 0-based cursor position.
 * @return 0-based index of the start of the previous word.
 */
std::size_t findPreviousWordStart(const std::string& buffer,
                                  std::size_t cursor) {
  const auto rbegin =
      buffer.rbegin() + static_cast<std::ptrdiff_t>(buffer.size() - cursor);
  const auto rend = buffer.rend();
  const auto afterSpaces = std::find_if_not(rbegin, rend, isSpace);
  const auto wordStart = std::find_if(afterSpaces, rend, isSpace);
  return static_cast<std::size_t>(rend - wordStart);
}

/**
 * @brief Return the index just past the end of the current or next word.
 *
 * Skips forwards over whitespace first, then over non-whitespace.
 *
 * @param buffer Input buffer to scan.
 * @param cursor Current 0-based cursor position.
 * @return 0-based index just past the end of the current or next word.
 */
std::size_t findNextWordEnd(const std::string& buffer, std::size_t cursor) {
  const auto begin = buffer.begin() + static_cast<std::ptrdiff_t>(cursor);
  const auto end = buffer.end();
  const auto afterSpaces = std::find_if_not(begin, end, isSpace);
  const auto wordEnd = std::find_if(afterSpaces, end, isSpace);
  return static_cast<std::size_t>(wordEnd - buffer.begin());
}

} // namespace

/**
 * @brief Mutable state of one `readLine` call.
 *
 * Local to a single call so each invocation starts from a clean slate.
 */
struct LineEditor::ReadLineState {
  /// Text of the line being edited right now.
  std::string buffer;

  /// 0-based position within `buffer` where the next typed character will be
  /// inserted.
  /// Valid range is `[0, buffer.size()]` (inclusive at the end so that
  /// appending past the last character is possible).
  std::size_t cursor{0};

  /// History-navigation mode.
  /// `nullopt` means "not in history, typing fresh text".
  /// When engaged, the value is the index inside `history` of the entry
  /// currently displayed: Up decrements it (older), Down increments it (newer);
  /// crossing past the newest entry resets it back to `nullopt`.
  std::optional<std::size_t> historyIndex;

  /// Snapshot of `buffer` taken the first time the user pressed Up in this
  /// call. Restored when Down navigates past the newest history entry, so the
  /// text the user was typing before entering history is not lost.
  std::string savedTypedBuffer;

  /// Set to `true` by a bound-key handler to request that `readLine` return
  /// immediately with `buffer` as the result, without waiting for Enter.
  bool submitted{false};
};

LineEditor::LineEditor(std::istream& in, std::ostream& out, std::string_view p)
    : input(in), output(out), prompt(p) {}

/// @brief Rewrite the current input line in place and reposition the cursor.
void LineEditor::redraw(const ReadLineState& state) const {
  output << '\r' << "\x1b[K" << prompt << state.buffer;
  const std::size_t targetColumn = prompt.size() + state.cursor + 1;
  output << '\r' << "\x1b[" << targetColumn << 'G';
  output.flush();
}

/// @brief Delete the character to the left of the cursor, if any.
void LineEditor::handleBackspace(ReadLineState& state) const {
  if (state.cursor > 0) {
    state.buffer.erase(state.cursor - 1, 1);
    --state.cursor;
    redraw(state);
  }
}

/// @brief Wipe the current line and place the cursor at column 0 (Ctrl+U).
void LineEditor::handleClearLine(ReadLineState& state) const {
  state.buffer.clear();
  state.cursor = 0;
  redraw(state);
}

/// @brief Delete the character under the cursor, if any (forward delete).
void LineEditor::handleDelete(ReadLineState& state) const {
  if (state.cursor < state.buffer.size()) {
    state.buffer.erase(state.cursor, 1);
    redraw(state);
  }
}

/// @brief Move the cursor one position to the left, if not already at 0.
void LineEditor::moveLeft(ReadLineState& state) const {
  if (state.cursor > 0) {
    --state.cursor;
    redraw(state);
  }
}

/// @brief Move the cursor one position to the right, if not past the end.
void LineEditor::moveRight(ReadLineState& state) const {
  if (state.cursor < state.buffer.size()) {
    ++state.cursor;
    redraw(state);
  }
}

/// @brief Move the cursor to the start of the line.
void LineEditor::moveHome(ReadLineState& state) const {
  state.cursor = 0;
  redraw(state);
}

/// @brief Move the cursor just past the last character of the line.
void LineEditor::moveEnd(ReadLineState& state) const {
  state.cursor = state.buffer.size();
  redraw(state);
}

/// @brief Move the cursor to the start of the previous word (Ctrl+Left).
void LineEditor::moveWordLeft(ReadLineState& state) const {
  state.cursor = findPreviousWordStart(state.buffer, state.cursor);
  redraw(state);
}

/// @brief Move the cursor to the end of the current or next word (Ctrl+Right).
void LineEditor::moveWordRight(ReadLineState& state) const {
  state.cursor = findNextWordEnd(state.buffer, state.cursor);
  redraw(state);
}

/// @brief Insert a printable character at the cursor position.
void LineEditor::insertChar(char c, ReadLineState& state) const {
  state.buffer.insert(state.cursor, 1, c);
  ++state.cursor;
  redraw(state);
}

/// @brief Recall the previous history entry into the buffer (Up).
/// The very first Up in a call snapshots the current typed buffer into
/// `savedTypedBuffer` so it can be restored later.
void LineEditor::recallOlder(ReadLineState& state) const {
  if (history.empty()) {
    return;
  }
  if (!state.historyIndex.has_value()) {
    state.savedTypedBuffer = state.buffer;
    state.historyIndex = history.size() - 1;
    state.buffer = history[*state.historyIndex];
  } else if (*state.historyIndex > 0) {
    --*state.historyIndex;
    state.buffer = history[*state.historyIndex];
  }
  state.cursor = state.buffer.size();
  redraw(state);
}

/// @brief Recall the next history entry into the buffer (Down).
/// Stepping past the newest entry restores `savedTypedBuffer` and leaves
/// history-navigation mode.
void LineEditor::recallNewer(ReadLineState& state) const {
  if (!state.historyIndex.has_value()) {
    return;
  }
  if (*state.historyIndex + 1 < history.size()) {
    ++*state.historyIndex;
    state.buffer = history[*state.historyIndex];
  } else {
    state.historyIndex.reset();
    state.buffer = state.savedTypedBuffer;
  }
  state.cursor = state.buffer.size();
  redraw(state);
}

/// @brief Consume a CSI escape sequence and dispatch to the matching handler.
/// Called after an `ESC` byte has been read from the input stream.
void LineEditor::handleEscape(ReadLineState& state) const {
  std::string params;
  char final = 0;
  if (!readCsi(input, params, final)) {
    return;
  }

  if (params.empty()) {
    switch (final) {
    case 'A': // Up
      recallOlder(state);
      break;
    case 'B': // Down
      recallNewer(state);
      break;
    case 'C': // Right
      moveRight(state);
      break;
    case 'D': // Left
      moveLeft(state);
      break;
    case 'H': // Home
      moveHome(state);
      break;
    case 'F': // End
      moveEnd(state);
      break;
    default:
      break;
    }
    return;
  }

  if (params == "3" && final == '~') { // Delete
    handleDelete(state);
    return;
  }
  if (params == "1;5" && final == 'D') { // Ctrl+Left
    moveWordLeft(state);
    return;
  }
  if (params == "1;5" && final == 'C') { // Ctrl+Right
    moveWordRight(state);
    return;
  }

  // Check the user-installed bindings (function keys, ...).
  const std::string key = params + final;
  if (const auto it = keyBindings.find(key); it != keyBindings.end()) {
    state.buffer = it->second;
    state.cursor = state.buffer.size();
    state.submitted = true;
  }
}

std::optional<std::string> LineEditor::readLine() {
  output << prompt;
  output.flush();

  ReadLineState state;

  while (true) {
    const int rawByte = input.get();
    if (rawByte == EOF) {
      return std::nullopt;
    }
    const auto c = static_cast<char>(rawByte);

    if (c == LF || c == CR) {
      output << '\n';
      output.flush();
      return state.buffer;
    }

    if (c == BACKSPACE_DEL || c == BACKSPACE_BS) {
      handleBackspace(state);
      continue;
    }

    if (c == CTRL_U) {
      handleClearLine(state);
      continue;
    }

    if (c == ESC) {
      handleEscape(state);
      if (state.submitted) {
        output << '\n';
        output.flush();
        return state.buffer;
      }
      continue;
    }

    insertChar(c, state);
  }
}

void LineEditor::addToHistory(std::string_view line) {
  history.emplace_back(line);
}

void LineEditor::bindKey(std::string_view csiSequence,
                         std::string_view command) {
  keyBindings.emplace(csiSequence, command);
}

} // namespace mqt::debugger
