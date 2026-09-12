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
 * @brief Small ANSI-aware rendering primitives used by the CLI front end.
 *
 * Exposes the ANSI escape constants and the string helpers that the CLI uses
 * to lay out its help bar, source view, and amplitude table.
 */

#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace mqt::debugger {

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

// Terminal control.
constexpr std::string_view ANSI_CLEAR_SCREEN = "\x1b[2J\x1b[1;1H";

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
 * @brief Return the widest cell in column @p col of @p rows.
 * @param rows Rows of cells. Every row must have at least @p col + 1 cells.
 * @param col Column index to measure.
 * @return The size of the largest cell in the requested column.
 */
size_t colMaxWidth(const std::vector<std::vector<std::string>>& rows,
                   size_t col);

/**
 * @brief Join @p parts with @p sep between adjacent parts.
 * @param parts The strings to join.
 * @param sep The separator to place between consecutive parts.
 * @return The joined string, or an empty string if @p parts is empty.
 */
std::string join(const std::vector<std::string>& parts, std::string_view sep);

/**
 * @brief Clear the terminal screen and place the cursor at the top-left.
 */
void clearScreen();

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
                const std::vector<std::vector<std::string>>& rows);

} // namespace mqt::debugger
