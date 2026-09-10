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
 * @file RawModeTerminal.hpp
 * @brief RAII guard that puts the terminal into raw input mode.
 */

#pragma once

#include <memory>

namespace mqt::debugger {

/**
 * @brief RAII guard that puts the terminal into raw input mode for its
 * lifetime and restores the previous mode on destruction.
 *
 * On POSIX, disables `ICANON` and `ECHO` on stdin so keystrokes reach the
 * program immediately, one byte at a time, without local echo.
 * On Windows, disables line input, echo input, and processed input on the
 * console handle, and enables both virtual-terminal input (so arrow keys
 * arrive as CSI escape sequences) and virtual-terminal output processing
 * (so ANSI escape sequences emitted by the program are honoured).
 *
 * Output processing (line-ending conversion) is left untouched, so the rest
 * of the program can keep using `std::cout << '\n'` without emitting `\r`.
 *
 * If stdin is not attached to a real terminal (e.g., redirected from a file
 * or a pipe), construction is a no-op and destruction restores nothing.
 * This makes the guard safe to use unconditionally.
 */
class RawModeTerminal {
public:
  RawModeTerminal();
  ~RawModeTerminal();

  RawModeTerminal(const RawModeTerminal&) = delete;
  RawModeTerminal& operator=(const RawModeTerminal&) = delete;
  RawModeTerminal(RawModeTerminal&&) = delete;
  RawModeTerminal& operator=(RawModeTerminal&&) = delete;

private:
  struct Impl;
  std::unique_ptr<Impl> impl;
};

} // namespace mqt::debugger
