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
 * `Ctrl+Right`, `Home`, `End`, `Delete`, `Up`, `Down`) and handles
 * `Backspace` and `Ctrl+U`.
 * An in-memory history is available via `Up`/`Down` after entries are added
 * with `addToHistory`.
 *
 * Terminal setup (raw mode / VT sequences) is the responsibility of the
 * caller. In production, pair this class with a `RawModeTerminal` scope.
 */
class LineEditor {
public:
  /**
   * @brief Construct the editor with input and output streams.
   * @param input The source of user input (typically `std::cin`).
   * @param output The sink for echoed characters and redraws (typically
   *               `std::cout`).
   */
  LineEditor(std::istream& input, std::ostream& output);

  ~LineEditor() = default;

  LineEditor(const LineEditor&) = delete;
  LineEditor& operator=(const LineEditor&) = delete;
  LineEditor(LineEditor&&) = delete;
  LineEditor& operator=(LineEditor&&) = delete;

  /**
   * @brief Read one line from the input stream, echoing the prompt first.
   * @param prompt The prompt to display before the input area.
   * @return The line entered by the user, without a trailing newline, or
   *         `std::nullopt` if end of input was reached before any newline.
   *         An engaged optional holding an empty string means the user
   *         pressed Enter on an empty buffer.
   */
  std::optional<std::string> readLine(std::string_view prompt);

  /**
   * @brief Add a line to the in-memory history, accessible via `Up`/`Down`.
   * @param line The line to remember.
   */
  void addToHistory(std::string_view line);

private:
  std::istream& input;
  std::ostream& output;
  std::vector<std::string> history;
};

} // namespace mqt::debugger
