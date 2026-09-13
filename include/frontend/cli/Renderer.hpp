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
 * @file Renderer.hpp
 * @brief ANSI-aware rendering primitives and a small class that funnels
 * CLI output to a configurable stream.
 */

#pragma once

#include <cstddef>
#include <iosfwd>
#include <string>
#include <string_view>
#include <vector>

namespace mqt::debugger {

namespace ansi {

// Style modifiers.
constexpr std::string_view RESET = "\x1b[0m";
constexpr std::string_view BOLD = "\x1b[1m";
constexpr std::string_view NORMAL = "\x1b[22m";

// Foreground colors.
constexpr std::string_view FG_BLACK = "\x1b[30m";
constexpr std::string_view FG_WHITE = "\x1b[97m";
constexpr std::string_view FG_CODE_DIM = "\x1b[90m";
constexpr std::string_view FG_CODE_HL = FG_BLACK;

// Background colors.
constexpr std::string_view BG_BREAKPOINT = "\x1b[41m";
constexpr std::string_view BG_CODE_HL = "\x1b[48;5;227m";
constexpr std::string_view BG_TABLE_HEADER = "\x1b[47m";
constexpr std::string_view BG_TABLE_TOP_ROW = "\x1b[48;5;153m";
constexpr std::string_view BG_TABLE_BOTTOM_ROW = "\x1b[44m";

// Terminal control.
constexpr std::string_view CLEAR_SCREEN = "\x1b[2J\x1b[1;1H";

} // namespace ansi

/**
 * @brief Wrap @p content between one space on each side.
 * @param content Text to wrap.
 * @return The wrapped string.
 */
std::string margins(std::string_view content);

/**
 * @brief Right-pad @p content with spaces up to @p width. Left-aligned.
 * @param content Text to pad.
 * @param width Target column width.
 * @return The left-aligned string, unchanged if it already reaches @p width.
 */
std::string leftAlign(std::string_view content, size_t width);

/**
 * @brief Left-pad @p content with spaces up to @p width. Right-aligned.
 * @param content Text to pad.
 * @param width Target column width.
 * @return The right-aligned string, unchanged if it already reaches @p width.
 */
std::string rightAlign(std::string_view content, size_t width);

/**
 * @brief Wrap @p content between bold on and bold off codes.
 * @param content Text to embolden.
 * @return The wrapped string.
 */
std::string bold(std::string_view content);

/**
 * @brief Wrap @p content between background color @p bg and a full reset.
 * @param content Text to paint.
 * @param bg ANSI background-color escape sequence.
 * @return The wrapped string.
 */
std::string bgColor(std::string_view content, std::string_view bg);

/**
 * @brief Wrap @p content between foreground color @p fg and a full reset.
 * @param content Text to paint.
 * @param fg ANSI foreground-color escape sequence.
 * @return The wrapped string.
 */
std::string fgColor(std::string_view content, std::string_view fg);

/**
 * @brief Join @p parts with @p sep between adjacent parts.
 * @param parts The strings to join.
 * @param sep The separator to place between consecutive parts.
 * @return The joined string, or an empty string if @p parts is empty.
 */
std::string join(const std::vector<std::string>& parts, std::string_view sep);

/**
 * @brief Funnels all CLI output through a single `std::ostream`.
 *
 * Owns nothing: the referenced stream must outlive the `Renderer`.
 * Constructed with `std::cout` in production and with an in-memory stream
 * in tests, so tests can capture exactly what the CLI would print.
 */
class Renderer {
public:
  /**
   * @brief Construct with the output stream to write to.
   * @param out The stream the CLI will render to. Must outlive this object.
   */
  explicit Renderer(std::ostream& out);

  ~Renderer() = default;

  Renderer(const Renderer&) = delete;
  Renderer& operator=(const Renderer&) = delete;
  Renderer(Renderer&&) = delete;
  Renderer& operator=(Renderer&&) = delete;

  /**
   * @brief Clear the terminal screen and place the cursor at the top-left.
   */
  void clearScreen();

  /**
   * @brief Write @p s to the output stream verbatim.
   * @param s The text to write.
   */
  void print(std::string_view s);

  /**
   * @brief Write @p s to the output stream followed by a newline.
   * @param s The text to write before the newline.
   */
  void println(std::string_view s);

private:
  std::ostream& out;
};

} // namespace mqt::debugger
