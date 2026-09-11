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
 * @file RawModeTerminalPosix.cpp
 * @brief POSIX implementation of `RawModeTerminal` using `termios`.
 */

#ifndef _WIN32

#include "frontend/cli/RawModeTerminal.hpp"

#include <memory>
#include <termios.h>
#include <unistd.h>

namespace mqt::debugger {

/// @brief Platform-specific state stored via PIMPL; restored on destruction.
struct RawModeTerminal::Impl {
  /// File descriptor put into raw mode; -1 means "no changes to restore".
  int fd{-1};
  /// Previous terminal settings, restored in the destructor.
  termios saved{};
};

RawModeTerminal::RawModeTerminal() : impl(std::make_unique<Impl>()) {
  const int fd = STDIN_FILENO;

  // If stdin is not a terminal (pipe, redirected file), leave it alone.
  // `impl->fd` stays -1 so the destructor is a no-op.
  if (isatty(fd) == 0) {
    return;
  }

  // Snapshot the current settings so we can restore them on destruction.
  if (tcgetattr(fd, &impl->saved) != 0) {
    return;
  }

  termios raw = impl->saved;
  // ICANON off: deliver each byte to the program instead of buffering a
  //             whole line until Enter, so arrow keys and Backspace can act
  //             immediately.
  // ECHO   off: the kernel no longer echoes typed bytes; `LineEditor` redraws
  //             the line itself, giving it full control over the visible
  //             cursor position.
  raw.c_lflag &= ~static_cast<tcflag_t>(ICANON | ECHO);
  // With ICANON off, VMIN/VTIME control how `read` blocks:
  //   VMIN = 1, VTIME = 0 -> return as soon as at least one byte is available,
  //                          no idle-time timeout.
  raw.c_cc[VMIN] = 1;
  raw.c_cc[VTIME] = 0;

  // TCSAFLUSH: wait for pending output to drain and discard pending input
  //            before switching, so any half-typed line is thrown away
  //            cleanly.
  if (tcsetattr(fd, TCSAFLUSH, &raw) != 0) {
    return;
  }
  impl->fd = fd;
}

RawModeTerminal::~RawModeTerminal() {
  if (impl->fd >= 0) {
    // Best effort: if the restore fails there is nothing sensible to do
    // from a destructor.
    tcsetattr(impl->fd, TCSAFLUSH, &impl->saved);
  }
}

} // namespace mqt::debugger

#endif // !_WIN32
