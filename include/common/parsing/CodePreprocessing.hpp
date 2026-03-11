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
 * @file CodePreprocessing.hpp
 * @brief Contains the definition of the functions used to preprocess the code
 * before the analysis and execution.
 */

#pragma once

#include "AssertionParsing.hpp"

#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace mqt::debugger {

/**
 * @brief Represents a block of code.
 *
 * Code blocks are typically given in curly braces after a custom gate
 * definition or an assertion.
 */
struct Block {
  /**
   * @brief Indicates whether the block is valid.
   *
   * After finishing to parse the block, this field is set to `true`. By
   * default, it is set to `false`.
   */
  bool valid;
  /**
   * @brief The code in the block.
   */
  std::string code;
};

/**
 * @brief Represents a preprocessed instruction in the code.
 *
 * Also contains additional meta-information about the instruction.
 */
struct Instruction {
  /**
   * @brief The instruction number of the instruction in the original code.
   *
   * Numbering starts at 0 and it typically coincides with the line number in
   * the code, but if multiple instructions are given in a single line, the
   * numbering is incremented by 1 for each instruction.
   */
  size_t lineNumber;

  /**
   * @brief The string representation of this code.
   */
  std::string code;

  /**
   * @brief The representation of this instruction as an assertion.
   *
   * If the instruction is an assertion, this field contains the parsed
   * assertion. Otherwise, it is `nullptr`.
   */
  std::unique_ptr<Assertion> assertion;

  /**
   * @brief The target variables or registers of this instruction.
   */
  std::vector<std::string> targets;

  /**
   * @brief The position of the instruction's string's first character in the
   * un-processed code.
   */
  size_t originalCodeStartPosition;

  /**
   * @brief The position of the instruction's string's last character in the
   * un-processed code.
   */
  size_t originalCodeEndPosition;

  /**
   * @brief The index of the successor instruction in the preprocessed code.
   *
   * Typically, this is `lineNumber + 1`, but it can be different if the
   * instruction is a call to a custom gate. If the instruction represents a
   * `return`, this value is set to 0, as the successor is not known at static
   * time.
   */
  size_t successorIndex;

  /**
   * @brief Indicates whether the instruction is a custom gate call.
   */
  bool isFunctionCall;

  /**
   * @brief The name of the custom gate called by this instruction, if it is a
   * custom gate call.
   */
  std::string calledFunction;

  /**
   * @brief Indicates whether the instruction is located inside a custom gate
   * definition.
   */
  bool inFunctionDefinition;

  /**
   * @brief Indicates whether the instruction represents a custom gate
   * definition.
   */
  bool isFunctionDefinition;

  /**
   * @brief The substitutions to be made in the code when calling a custom gate.
   *
   * Maps from the names of the parameters of the custom gate to
   * the names of the variables used to call the custom gate.
   */
  std::map<std::string, std::string> callSubstitution;

  /**
   * @brief The immediate data dependencies of this instruction.
   *
   * For each variable used by this instruction, this vector contains a
   * reference to the last instruction that used it. It also contains the index
   * of the variable in that dependency's argument list, so that it can be
   * identified exactly.
   */
  std::vector<std::pair<size_t, size_t>> dataDependencies;

  /**
   * @brief The block of code following this instruction.
   *
   * If the instruction is a custom gate definition or an assertion, this block
   * contains the code in the curly braces following the definition or
   * assertion. Otherwise, it is an empty block instance with `valid = false`.
   */
  Block block;

  /**
   * @brief The indices of the child instructions of this instruction.
   *
   * If the instruction is a custom gate definition, this vector contains the
   * indices of the instructions that are part of the custom gate. Otherwise, it
   * is an empty vector.
   */
  std::vector<size_t> childInstructions;

  /**
   * @brief Constructs a new Instruction object.
   * @param inputLineNumber The instruction number of the instruction in the
   * original code.
   * @param inputCode The string representation of this code.
   * @param inputAssertion The representation of this instruction as an
   * assertion, or `nullptr`.
   * @param inputTargets The target variables or registers of this instruction.
   * @param startPos The position of the instruction's string's first character
   * in the un-processed code.
   * @param endPos The position of the instruction's string's last character in
   * the un-processed code.
   * @param successor The index of the successor instruction in the preprocessed
   * code.
   * @param isFuncCall True if the instruction is a custom gate call.
   * @param function The name of the custom gate called by this instruction, if
   * it is a custom gate call.
   * @param inFuncDef True if the instruction is located inside a custom gate
   * definition.
   * @param isFuncDef True if the instruction represents a custom gate
   * definition.
   * @param inputBlock The block of code following this instruction.
   */
  Instruction(size_t inputLineNumber, std::string inputCode,
              std::unique_ptr<Assertion>& inputAssertion,
              std::vector<std::string> inputTargets, size_t startPos,
              size_t endPos, size_t successor, bool isFuncCall,
              std::string function, bool inFuncDef, bool isFuncDef,
              Block inputBlock);
};

/**
 * @brief Represents a classic-controlled gate in the code.
 *
 * Classic-controlled gates are defined using the `if` keyword with one of the
 * following formats:\n `if (condition) operation;` or `if (condition) {
 * operation1; operation2; ... }`
 */
struct ClassicControlledGate {
  /**
   * @brief The condition of the classic-controlled gate.
   */
  std::string condition;
  /**
   * @brief The quantum operations to perform if the condition is met.
   */
  std::vector<std::string> operations;
};

/**
 * @brief Represents a parsed classic-controlled condition.
 */
struct ClassicCondition {
  /**
   * @brief The name of the classical register.
   */
  std::string registerName;
  /**
   * @brief Optional bit index if the condition targets a single bit.
   */
  std::optional<size_t> bitIndex;
  /**
   * @brief The expected value in the condition comparison.
   */
  size_t expectedValue;
};

/**
 * @brief Represents a function definition in the code.
 */
struct FunctionDefinition {
  /**
   * @brief The name of the function.
   */
  std::string name;
  /**
   * @brief The parameter names of the function.
   */
  std::vector<std::string> parameters;
};

/**
 * @brief Process a given source code and return a vector of instructions.
 *
 * The processed code is stored in the `processedCode` parameter.
 *
 * @param code The source code to process.
 * @param processedCode A reference to store the processed code in.
 * @return A vector of all processed instructions.
 */
std::vector<Instruction> preprocessCode(const std::string& code,
                                        std::string& processedCode);

/**
 * @brief Process a given code block and return a vector of instructions.
 * @param code The source code to process.
 * @param startIndex The line number index from which this block starts in the
 * full code.
 * @param initialCodeOffset The index of the character in the full source code
 * string, from which this block starts.
 * @param functionNames The names of the functions defined in the previous
 * scope.
 * @param definedRegisters The registers defined in the previous scope.
 * @param shadowedRegisters The registers shadowed by the current or previous
 * scopes (e.g., by having function parameters with the same name).
 * @param processedCode A reference to store the processed code in.
 * @return A vector of all processed instructions.
 */
std::vector<Instruction>
preprocessCode(const std::string& code, size_t startIndex,
               size_t initialCodeOffset,
               const std::vector<std::string>& functionNames,
               std::map<std::string, size_t>& definedRegisters,
               const std::vector<std::string>& shadowedRegisters,
               std::string& processedCode);

/**
 * @brief Check if a given line is a function definition.
 *
 * This is done by checking if it starts with `gate `.
 * @param line The line to check.
 * @return True if the line is a function definition, false otherwise.
 */
bool isFunctionDefinition(const std::string& line);

/**
 * @brief Check if a given line is a classic controlled gate.
 *
 * This is done by checking if it starts with `if` and contains parentheses.
 * @param line The line to check.
 * @return True if the line is a classic controlled gate, false otherwise.
 */
bool isClassicControlledGate(const std::string& line);

/**
 * @brief Parse a classic-controlled gate from a given line (or multiple lines)
 * of code.
 * @param code The code to parse.
 * @return The parsed classic-controlled gate.
 */
ClassicControlledGate parseClassicControlledGate(const std::string& code);

/**
 * @brief Parse a classic-controlled condition expression.
 * @param condition The condition string to parse.
 * @return The parsed condition, or std::nullopt if it cannot be parsed.
 */
std::optional<ClassicCondition>
parseClassicConditionExpression(const std::string& condition);

/**
 * @brief Parse a classic-controlled condition from a classic-controlled gate.
 * @param code The code to parse.
 * @return The parsed condition, or std::nullopt if it cannot be parsed.
 */
std::optional<ClassicCondition>
parseClassicConditionFromCode(const std::string& code);

/**
 * @brief Check if a given line is a variable declaration.
 *
 * This is done by checking if it contains `creg ` or `qreg `.
 * @param line The line to check.
 * @return True if the line is a variable declaration, false otherwise.
 */
bool isVariableDeclaration(const std::string& line);

/**
 * @brief Check if a given line is a measurement.
 *
 * This is done by checking if it contains the symbols `->`.
 * @param line The line to check.
 * @return True if the line is a measurement, false otherwise.
 */
bool isMeasurement(const std::string& line);

/**
 * @brief Check if a given line is a reset operation.
 *
 * This is done by checking if it starts with `reset `.
 * @param line The line to check.
 * @return True if the line is a reset operation, false otherwise.
 */
bool isReset(const std::string& line);

/**
 * @brief Check if a given line is a barrier operation.
 *
 * This is done by checking if it starts with `barrier ` or `barrier;`.
 * @param line The line to check.
 * @return True if the line is a barrier operation, false otherwise.
 */
bool isBarrier(const std::string& line);

/**
 * @brief Parse the parameters or arguments from a given instruction.
 * @param instruction The instruction to parse.
 * @return A vector containing the parsed parameters.
 */
std::vector<std::string> parseParameters(const std::string& instruction);

} // namespace mqt::debugger
