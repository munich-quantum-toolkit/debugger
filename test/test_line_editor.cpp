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
 * @file test_line_editor.cpp
 * @brief Unit tests for the `LineEditor` CLI helper.
 *
 * The editor is driven from in-memory `std::stringstream` buffers so that
 * every case can be described as an input byte sequence and an expected
 * output line, with no dependency on a real terminal.
 */

#include "frontend/cli/LineEditor.hpp"

#include <gtest/gtest.h>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

namespace mqt::debugger::test {

namespace {

constexpr std::string_view DEFAULT_PROMPT = "> ";

/**
 * @brief Read one line with a fresh editor and a given input string.
 */
std::optional<std::string>
readLineWith(const std::string& inputBytes,
             std::string_view prompt = DEFAULT_PROMPT) {
  std::stringstream input(inputBytes);
  std::stringstream output;
  const LineEditor editor{input, output, prompt};
  return editor.readLine();
}

} // namespace

//
// Prompt
//

TEST(LineEditorTest, WritesPromptToOutput) {
  std::stringstream input("hello\n");
  std::stringstream output;
  const LineEditor editor{input, output, DEFAULT_PROMPT};
  static_cast<void>(editor.readLine());
  EXPECT_TRUE(output.str().starts_with(DEFAULT_PROMPT));
}

//
// EOF, CR, LF, control bytes
//

TEST(LineEditorTest, NulloptOnEof) {
  EXPECT_EQ(readLineWith(""), std::nullopt);
}

TEST(LineEditorTest, EmptyLineOnJustEnter) {
  EXPECT_EQ(readLineWith("\n"), "");
}

TEST(LineEditorTest, ReadsPlainLineTerminatedByNewline) {
  EXPECT_EQ(readLineWith("hello\n"), "hello");
}

TEST(LineEditorTest, ReadsPlainLineTerminatedByCarriageReturn) {
  EXPECT_EQ(readLineWith("hello\r"), "hello");
}

TEST(LineEditorTest, CrlfIsConsumedAsSingleEnterAcrossReadLineCalls) {
  std::stringstream input("first\r\nsecond\n");
  std::stringstream output;
  const LineEditor editor{input, output, DEFAULT_PROMPT};
  EXPECT_EQ(editor.readLine(), "first");
  EXPECT_EQ(editor.readLine(), "second");
}

TEST(LineEditorTest, UnhandledControlByteIsIgnored) {
  // Tab (0x09) has no binding and is not a recognized editor control.
  EXPECT_EQ(readLineWith("a\tb\n"), "ab");
}

//
// Delete, Backspace, Ctrl+D, Ctrl+U
//

TEST(LineEditorTest, DeleteRemovesCharacterUnderCursor) {
  EXPECT_EQ(readLineWith("abc\x1b[D\x1b[D\x1b[3~\n"), "ac");
}

TEST(LineEditorTest, BackspaceRemovesPreviousCharacter) {
  EXPECT_EQ(readLineWith("he\x7fllo\n"), "hllo");
}

TEST(LineEditorTest, BackspaceOnEmptyBufferIsNoop) {
  EXPECT_EQ(readLineWith("\x7f\x7fhi\n"), "hi");
}

TEST(LineEditorTest, CtrlDOnEmptyBufferReturnsNullopt) {
  EXPECT_EQ(readLineWith("\x04"), std::nullopt);
}

TEST(LineEditorTest, CtrlDOnNonEmptyBufferIsIgnored) {
  EXPECT_EQ(readLineWith("hi\x04\n"), "hi");
}

TEST(LineEditorTest, CtrlUClearsTheLine) {
  EXPECT_EQ(readLineWith("abc\x15xyz\n"), "xyz");
}

//
// Cursor navigation (Left / Right / Home / End)
//

TEST(LineEditorTest, CursorLeftAllowsInsertionInTheMiddle) {
  EXPECT_EQ(readLineWith("ab\x1b[DX\n"), "aXb");
}

TEST(LineEditorTest, CursorRightMovesTowardsEnd) {
  EXPECT_EQ(readLineWith("ab\x1b[D\x1b[D\x1b[CX\n"), "aXb");
}

TEST(LineEditorTest, HomeMovesToStart) {
  EXPECT_EQ(readLineWith("abc\x1b[HX\n"), "Xabc");
}

TEST(LineEditorTest, HomeAlsoAcceptsCsi1Tilde) {
  EXPECT_EQ(readLineWith("abc\x1b[1~X\n"), "Xabc");
}

TEST(LineEditorTest, HomeAlsoAcceptsCsi7Tilde) {
  EXPECT_EQ(readLineWith("abc\x1b[7~X\n"), "Xabc");
}

TEST(LineEditorTest, EndMovesToPastLast) {
  EXPECT_EQ(readLineWith("abc\x1b[H\x1b[FX\n"), "abcX");
}

TEST(LineEditorTest, EndAlsoAcceptsCsi4Tilde) {
  EXPECT_EQ(readLineWith("abc\x1b[H\x1b[4~X\n"), "abcX");
}

TEST(LineEditorTest, EndAlsoAcceptsCsi8Tilde) {
  EXPECT_EQ(readLineWith("abc\x1b[H\x1b[8~X\n"), "abcX");
}

//
// Word navigation (Ctrl+Left / Ctrl+Right)
//

TEST(LineEditorTest, CtrlLeftJumpsToPreviousWord) {
  // "hello world" + Ctrl+Left + "X" + Enter -> "hello Xworld"
  EXPECT_EQ(readLineWith("hello world\x1b[1;5DX\n"), "hello Xworld");
}

TEST(LineEditorTest, CtrlRightJumpsToNextWord) {
  // "hello world" + Home + Ctrl+Right + "X" + Enter -> "helloX world"
  EXPECT_EQ(readLineWith("hello world\x1b[H\x1b[1;5CX\n"), "helloX world");
}

//
// History (Up / Down)
//

TEST(LineEditorTest, UpArrowRecallsPreviousHistoryEntry) {
  std::stringstream input("\x1b[A\n");
  std::stringstream output;
  LineEditor editor{input, output, DEFAULT_PROMPT};
  editor.addToHistory("previous");
  EXPECT_EQ(editor.readLine(), "previous");
}

TEST(LineEditorTest, UpAndDownArrowsNavigateHistory) {
  std::stringstream input("\x1b[A\x1b[A\x1b[B\n");
  std::stringstream output;
  LineEditor editor{input, output, DEFAULT_PROMPT};
  editor.addToHistory("first");
  editor.addToHistory("second");
  // Up -> "second", Up -> "first", Down -> "second".
  EXPECT_EQ(editor.readLine(), "second");
}

TEST(LineEditorTest, UpArrowOnEmptyHistoryIsNoop) {
  EXPECT_EQ(readLineWith("\x1b[Ahi\n"), "hi");
}

TEST(LineEditorTest, UpArrowStopsAtOldestEntry) {
  // Only one entry: many Ups still yield that same entry.
  std::stringstream input("\x1b[A\x1b[A\x1b[A\n");
  std::stringstream output;
  LineEditor editor{input, output, DEFAULT_PROMPT};
  editor.addToHistory("only");
  EXPECT_EQ(editor.readLine(), "only");
}

TEST(LineEditorTest, DownArrowOnEmptyHistoryIsNoop) {
  EXPECT_EQ(readLineWith("\x1b[Bhi\n"), "hi");
}

TEST(LineEditorTest, DownArrowBeyondNewestReturnsToTypedBuffer) {
  // Type "typed", Up (recalls "old"), Down (goes back to "typed"), Enter.
  std::stringstream input("typed\x1b[A\x1b[B\n");
  std::stringstream output;
  LineEditor editor{input, output, DEFAULT_PROMPT};
  editor.addToHistory("old");
  EXPECT_EQ(editor.readLine(), "typed");
}

//
// Key bindings (function keys)
//

TEST(LineEditorTest, BoundKeyAutoSubmitsWithoutEnter) {
  // F5 is CSI 15~; no Enter is sent.
  std::stringstream input("\x1b[15~");
  std::stringstream output;
  LineEditor editor{input, output, DEFAULT_PROMPT};
  editor.bindKey("15~", "run");
  EXPECT_EQ(editor.readLine(), "run");
}

TEST(LineEditorTest, BoundKeyReplacesTypedBuffer) {
  // User types "get x" then presses F5, which is bound to "run".
  std::stringstream input("get x\x1b[15~");
  std::stringstream output;
  LineEditor editor{input, output, DEFAULT_PROMPT};
  editor.bindKey("15~", "run");
  EXPECT_EQ(editor.readLine(), "run");
}

TEST(LineEditorTest, UnboundCsiSequenceIsIgnored) {
  // F1 (CSI 11~) has no binding, so the editor keeps waiting;
  // the "hi\n" that follows is what actually terminates the line.
  std::stringstream input("\x1b[11~hi\n");
  std::stringstream output;
  LineEditor editor{input, output, DEFAULT_PROMPT};
  editor.bindKey("15~", "run"); // unrelated binding, must not fire on F1
  EXPECT_EQ(editor.readLine(), "hi");
}

} // namespace mqt::debugger::test
