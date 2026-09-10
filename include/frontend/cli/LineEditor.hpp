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
 * @file LineEditor.hpp
 * @brief A minimal line editor for the debugger CLI.
 */

#pragma once

#include <istream>
#include <map>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace mqt::debugger {

/**
 * @brief A minimal line editor that reads one line at a time from an
 * `std::istream` and echoes to an `std::ostream`.
 *
 * The editor is agnostic to whether the underlying streams are backed by a
 * real terminal (in raw mode) or by in-memory buffers used from tests.
 * It parses the common CSI escape sequences (`Left`, `Right`, `Ctrl+Left`,
 * `Ctrl+Right`, `Home`, `End`, `Delete`, `Up`, `Down`) and handles `Backspace`
 * and `Ctrl+U`.
 * An in-memory history is available via `Up`/`Down` after entries are added
 * with `addToHistory`.
 *
 * Terminal setup (raw mode / VT sequences) is the responsibility of the caller.
 * In production, pair this class with a `RawModeTerminal` scope.
 */
class LineEditor {
public:
  /**
   * @brief Construct the editor with input and output streams and a prompt.
   * @param input The source of user input (typically `std::cin`).
   * @param output The sink for echoed characters and redraws (typically
   * `std::cout`).
   * @param prompt The prompt to display before the input area.
   */
  LineEditor(std::istream& input, std::ostream& output,
             std::string_view prompt);

  ~LineEditor() = default;

  LineEditor(const LineEditor&) = delete;
  LineEditor& operator=(const LineEditor&) = delete;
  LineEditor(LineEditor&&) = delete;
  LineEditor& operator=(LineEditor&&) = delete;

  /**
   * @brief Read one line from the input stream, echoing the prompt first.
   * @return The line entered by the user, without a trailing newline, or
   * `std::nullopt` if end of input was reached before any newline. An engaged
   * optional holding an empty string means the user pressed Enter on an empty
   * buffer.
   */
  std::optional<std::string> readLine();

  /**
   * @brief Add a line to the in-memory history, accessible via `Up`/`Down`.
   * @param line The line to remember.
   */
  void addToHistory(std::string_view line);

  /**
   * @brief Bind a CSI escape sequence to a command string.
   *
   * When the terminal delivers a CSI sequence whose parameter bytes and final
   * byte match the ones registered here, `readLine` replaces the current buffer
   * with @p command and returns immediately, as if the user had typed
   * @p command and pressed Enter.
   *
   * Typical use is binding function keys (`F5`, `F6`, ...) to debugger
   * commands.
   *
   * @param csiSequence Parameter bytes followed by the final byte of the CSI
   * sequence (for example `"15~"` for `F5`).
   * @param command The command string to auto-submit when the sequence is
   * received.
   */
  void bindKey(std::string_view csiSequence, std::string_view command);

private:
  struct ReadLineState;

  void redraw(const ReadLineState& state) const;

  void handleBackspace(ReadLineState& state) const;
  void handleClearLine(ReadLineState& state) const;
  void handleDelete(ReadLineState& state) const;
  void moveLeft(ReadLineState& state) const;
  void moveRight(ReadLineState& state) const;
  void moveHome(ReadLineState& state) const;
  void moveEnd(ReadLineState& state) const;
  void moveWordLeft(ReadLineState& state) const;
  void moveWordRight(ReadLineState& state) const;
  void insertChar(char c, ReadLineState& state) const;
  void recallOlder(ReadLineState& state) const;
  void recallNewer(ReadLineState& state) const;
  void handleEscape(ReadLineState& state) const;

  std::istream& input;
  std::ostream& output;
  std::string prompt;
  std::vector<std::string> history;
  std::map<std::string, std::string> keyBindings;
};

} // namespace mqt::debugger
