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

bool isSpace(char c) {
  return std::isspace(static_cast<unsigned char>(c)) != 0;
}

/**
 * @brief Rewrite the current input line in place on `out`.
 *
 * Moves the cursor to the start of the current line, clears to end of line,
 * prints the prompt followed by the buffer, then repositions the cursor via
 * the `CSI n G` (Cursor Horizontal Absolute) escape sequence.
 *
 * @param out    Output stream to write to.
 * @param prompt Prompt to display before the buffer.
 * @param buffer Current input line contents.
 * @param cursor 0-based cursor position within the buffer.
 */
void redraw(std::ostream& out, std::string_view prompt,
            const std::string& buffer, std::size_t cursor) {
  out << '\r' << "\x1b[K" << prompt << buffer;
  const std::size_t targetColumn = prompt.size() + cursor + 1;
  out << '\r' << "\x1b[" << targetColumn << 'G';
  out.flush();
}

/**
 * @brief Read a CSI (Control Sequence Introducer) escape sequence after
 * `ESC` has been consumed.
 *
 * A CSI sequence has the form `ESC [ params final`, where `params` are the
 * parameter bytes (may be empty) and `final` is a letter or `~` that
 * identifies the command.
 *
 * @param in     Input stream, positioned just after the `ESC` byte.
 * @param params Out parameter: parameter bytes of the sequence.
 * @param final  Out parameter: final byte of the sequence.
 * @returns true on a well-formed sequence; false if it is malformed or the
 *          stream ends mid-sequence.
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
 * @returns 0-based index of the start of the previous word.
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
 * @returns 0-based index just past the end of the current or next word.
 */
std::size_t findNextWordEnd(const std::string& buffer, std::size_t cursor) {
  const auto begin = buffer.begin() + static_cast<std::ptrdiff_t>(cursor);
  const auto end = buffer.end();
  const auto afterSpaces = std::find_if_not(begin, end, isSpace);
  const auto wordEnd = std::find_if(afterSpaces, end, isSpace);
  return static_cast<std::size_t>(wordEnd - buffer.begin());
}

} // namespace

LineEditor::LineEditor(std::istream& in, std::ostream& out)
    : input(in), output(out) {}

std::optional<std::string> LineEditor::readLine(std::string_view prompt) {
  output << prompt;
  output.flush();

  std::string buffer;
  std::size_t cursor = 0;
  std::optional<std::size_t> historyIndex;
  std::string savedTypedBuffer;

  while (true) {
    const int rawByte = input.get();
    if (rawByte == EOF) {
      return std::nullopt;
    }
    const auto c = static_cast<char>(rawByte);

    if (c == LF || c == CR) {
      output << '\n';
      output.flush();
      return buffer;
    }

    if (c == BACKSPACE_DEL || c == BACKSPACE_BS) {
      if (cursor > 0) {
        buffer.erase(cursor - 1, 1);
        --cursor;
        redraw(output, prompt, buffer, cursor);
      }
      continue;
    }

    if (c == CTRL_U) {
      buffer.clear();
      cursor = 0;
      redraw(output, prompt, buffer, cursor);
      continue;
    }

    if (c == ESC) {
      std::string params;
      char final = 0;
      if (!readCsi(input, params, final)) {
        continue;
      }

      if (params.empty()) {
        switch (final) {
        case 'A': // Up
          if (!history.empty()) {
            if (!historyIndex.has_value()) {
              savedTypedBuffer = buffer;
              historyIndex = history.size() - 1;
              buffer = history[*historyIndex];
            } else if (*historyIndex > 0) {
              --*historyIndex;
              buffer = history[*historyIndex];
            }
            cursor = buffer.size();
            redraw(output, prompt, buffer, cursor);
          }
          break;
        case 'B': // Down
          if (historyIndex.has_value()) {
            if (*historyIndex + 1 < history.size()) {
              ++*historyIndex;
              buffer = history[*historyIndex];
            } else {
              historyIndex.reset();
              buffer = savedTypedBuffer;
            }
            cursor = buffer.size();
            redraw(output, prompt, buffer, cursor);
          }
          break;
        case 'C': // Right
          if (cursor < buffer.size()) {
            ++cursor;
            redraw(output, prompt, buffer, cursor);
          }
          break;
        case 'D': // Left
          if (cursor > 0) {
            --cursor;
            redraw(output, prompt, buffer, cursor);
          }
          break;
        case 'H': // Home
          cursor = 0;
          redraw(output, prompt, buffer, cursor);
          break;
        case 'F': // End
          cursor = buffer.size();
          redraw(output, prompt, buffer, cursor);
          break;
        default:
          break;
        }
      } else if (params == "3" && final == '~') { // Delete
        if (cursor < buffer.size()) {
          buffer.erase(cursor, 1);
          redraw(output, prompt, buffer, cursor);
        }
      } else if (params == "1;5" && final == 'D') { // Ctrl+Left
        cursor = findPreviousWordStart(buffer, cursor);
        redraw(output, prompt, buffer, cursor);
      } else if (params == "1;5" && final == 'C') { // Ctrl+Right
        cursor = findNextWordEnd(buffer, cursor);
        redraw(output, prompt, buffer, cursor);
      }
      continue;
    }

    // Regular character: insert at cursor position.
    buffer.insert(cursor, 1, c);
    ++cursor;
    redraw(output, prompt, buffer, cursor);
  }
}

void LineEditor::addToHistory(std::string_view line) {
  history.emplace_back(line);
}

} // namespace mqt::debugger
