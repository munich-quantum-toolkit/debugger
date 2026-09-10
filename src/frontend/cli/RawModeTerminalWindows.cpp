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
 * @file RawModeTerminalWindows.cpp
 * @brief Windows implementation of `RawModeTerminal` using `SetConsoleMode`.
 */

#ifdef _WIN32

#include "frontend/cli/RawModeTerminal.hpp"

#include <memory>
#include <windows.h>

namespace mqt::debugger {

struct RawModeTerminal::Impl {
  HANDLE hIn{INVALID_HANDLE_VALUE};
  HANDLE hOut{INVALID_HANDLE_VALUE};
  DWORD savedIn{0};
  DWORD savedOut{0};
  bool restoreIn{false};
  bool restoreOut{false};
};

RawModeTerminal::RawModeTerminal() : impl(std::make_unique<Impl>()) {
  impl->hIn = GetStdHandle(STD_INPUT_HANDLE);
  impl->hOut = GetStdHandle(STD_OUTPUT_HANDLE);

  // `GetConsoleMode` fails when the handle is not a console (e.g., stdin
  // has been redirected from a file or pipe); in that case leave the
  // corresponding `restore*` flag false so the destructor skips it.
  if (impl->hIn != INVALID_HANDLE_VALUE &&
      GetConsoleMode(impl->hIn, &impl->savedIn) != 0) {
    DWORD raw = impl->savedIn;
    // ENABLE_LINE_INPUT      off: return input one keystroke at a time
    //                             instead of a whole line at Enter.
    // ENABLE_ECHO_INPUT      off: the console no longer echoes typed
    //                             characters; `LineEditor` handles the
    //                             visible line itself.
    // ENABLE_PROCESSED_INPUT off: keep Ctrl+C / Ctrl+Z from being handled
    //                             by the console before we see them.
    raw &= ~static_cast<DWORD>(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT |
                               ENABLE_PROCESSED_INPUT);
    // Available since Windows 10 build 1809: arrow keys and other special
    // keys arrive as ANSI/VT escape sequences instead of Windows-specific
    // input records, matching what `LineEditor` parses on POSIX.
    raw |= ENABLE_VIRTUAL_TERMINAL_INPUT;
    if (SetConsoleMode(impl->hIn, raw) != 0) {
      impl->restoreIn = true;
    }
  }

  if (impl->hOut != INVALID_HANDLE_VALUE &&
      GetConsoleMode(impl->hOut, &impl->savedOut) != 0) {
    // ENABLE_VIRTUAL_TERMINAL_PROCESSING on: the console interprets ANSI
    // escape sequences we emit (cursor moves, clear-line, colors) instead
    // of printing them literally.
    const DWORD cooked = impl->savedOut | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (SetConsoleMode(impl->hOut, cooked) != 0) {
      impl->restoreOut = true;
    }
  }
}

RawModeTerminal::~RawModeTerminal() {
  // Best effort: nothing sensible to do from a destructor if a restore fails.
  if (impl->restoreIn) {
    SetConsoleMode(impl->hIn, impl->savedIn);
  }
  if (impl->restoreOut) {
    SetConsoleMode(impl->hOut, impl->savedOut);
  }
}

} // namespace mqt::debugger

#endif // _WIN32
