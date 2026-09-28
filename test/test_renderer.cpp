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
 * @file test_renderer.cpp
 * @brief Unit tests for the `Renderer` output class and the free rendering
 * helpers in `Renderer.hpp`.
 *
 * The `Renderer` is driven from an in-memory `std::ostringstream` so tests
 * can assert on the exact bytes it emits.
 */

#include "frontend/cli/Renderer.hpp"

#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace mqt::debugger::test {

namespace {

/**
 * @brief Return `prefix + content + suffix` as a `std::string`.
 */
std::string wrap(std::string_view prefix, std::string_view content,
                 std::string_view suffix) {
  std::string s{prefix};
  s.append(content);
  s.append(suffix);
  return s;
}

} // namespace

//
// Free helpers
//

TEST(RendererHelpersTest, MarginsWrapsWithOneSpaceEachSide) {
  EXPECT_EQ(margins("hello"), " hello ");
}

TEST(RendererHelpersTest, MarginsOnEmptyIsTwoSpaces) {
  EXPECT_EQ(margins(""), "  ");
}

TEST(RendererHelpersTest, LeftAlignPadsRightUpToWidth) {
  EXPECT_EQ(leftAlign("hi", 5), "hi   ");
}

TEST(RendererHelpersTest, LeftAlignReturnsInputUnchangedWhenAlreadyWider) {
  EXPECT_EQ(leftAlign("hello", 3), "hello");
}

TEST(RendererHelpersTest, LeftAlignReturnsInputUnchangedAtExactWidth) {
  EXPECT_EQ(leftAlign("hello", 5), "hello");
}

TEST(RendererHelpersTest, LeftAlignOnEmptyIsAllSpaces) {
  EXPECT_EQ(leftAlign("", 3), "   ");
}

TEST(RendererHelpersTest, RightAlignPadsLeftUpToWidth) {
  EXPECT_EQ(rightAlign("hi", 5), "   hi");
}

TEST(RendererHelpersTest, RightAlignReturnsInputUnchangedWhenAlreadyWider) {
  EXPECT_EQ(rightAlign("hello", 3), "hello");
}

TEST(RendererHelpersTest, RightAlignReturnsInputUnchangedAtExactWidth) {
  EXPECT_EQ(rightAlign("hello", 5), "hello");
}

TEST(RendererHelpersTest, RightAlignOnEmptyIsAllSpaces) {
  EXPECT_EQ(rightAlign("", 3), "   ");
}

TEST(RendererHelpersTest, BoldWrapsBetweenBoldOnAndOffCodes) {
  EXPECT_EQ(bold("hi"), wrap(ansi::BOLD, "hi", ansi::NORMAL));
}

TEST(RendererHelpersTest, BoldOnEmptyStillEmitsBothCodes) {
  EXPECT_EQ(bold(""), wrap(ansi::BOLD, "", ansi::NORMAL));
}

TEST(RendererHelpersTest, BgColorWrapsBetweenGivenBgAndReset) {
  EXPECT_EQ(bgColor("hi", ansi::BG_BREAKPOINT),
            wrap(ansi::BG_BREAKPOINT, "hi", ansi::RESET));
}

TEST(RendererHelpersTest, FgColorWrapsBetweenGivenFgAndReset) {
  EXPECT_EQ(fgColor("hi", ansi::FG_BLACK),
            wrap(ansi::FG_BLACK, "hi", ansi::RESET));
}

TEST(RendererHelpersTest, JoinOfEmptyReturnsEmpty) {
  const std::vector<std::string> parts;
  EXPECT_EQ(join(parts, "-"), "");
}

TEST(RendererHelpersTest, JoinOfOneReturnsThatOne) {
  const std::vector<std::string> parts{"a"};
  EXPECT_EQ(join(parts, "-"), "a");
}

TEST(RendererHelpersTest, JoinInsertsSeparatorBetweenAdjacentParts) {
  const std::vector<std::string> parts{"a", "b", "c"};
  EXPECT_EQ(join(parts, ", "), "a, b, c");
}

TEST(RendererHelpersTest, JoinWithEmptySeparatorConcatenates) {
  const std::vector<std::string> parts{"a", "b"};
  EXPECT_EQ(join(parts, ""), "ab");
}

//
// Renderer class
//

TEST(RendererTest, ClearScreenEmitsAnsiClearScreenSequence) {
  std::ostringstream out;
  Renderer r{out};
  r.clearScreen();
  EXPECT_EQ(out.str(), ansi::CLEAR_SCREEN);
}

TEST(RendererTest, PrintWritesVerbatim) {
  std::ostringstream out;
  Renderer r{out};
  r.print("hello");
  EXPECT_EQ(out.str(), "hello");
}

TEST(RendererTest, PrintOfEmptyWritesNothing) {
  std::ostringstream out;
  Renderer r{out};
  r.print("");
  EXPECT_EQ(out.str(), "");
}

TEST(RendererTest, PrintlnWritesInputFollowedByNewline) {
  std::ostringstream out;
  Renderer r{out};
  r.println("hello");
  EXPECT_EQ(out.str(), "hello\n");
}

TEST(RendererTest, PrintlnOfEmptyWritesJustNewline) {
  std::ostringstream out;
  Renderer r{out};
  r.println("");
  EXPECT_EQ(out.str(), "\n");
}

TEST(RendererTest, ConsecutiveWritesAccumulate) {
  std::ostringstream out;
  Renderer r{out};
  r.print("foo");
  r.println(" bar");
  r.print("baz");
  EXPECT_EQ(out.str(), "foo bar\nbaz");
}

} // namespace mqt::debugger::test
