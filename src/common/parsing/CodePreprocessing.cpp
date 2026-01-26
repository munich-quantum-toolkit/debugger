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
 * @file CodePreprocessing.cpp
 * @brief Implementation of the code preprocessing functionality.
 */

#include "common/parsing/CodePreprocessing.hpp"

#include "common/parsing/AssertionParsing.hpp"
#include "common/parsing/ParsingError.hpp"
#include "common/parsing/Utils.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <exception>
#include <iterator>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace mqt::debugger {

namespace {

/**
 * @brief Check whether a string is non-empty and contains only digits.
 * @param text The string to validate.
 * @return True if the string is non-empty and all characters are digits.
 */
bool isDigits(const std::string& text) {
  if (text.empty()) {
    return false;
  }
  return std::ranges::all_of(
      text, [](unsigned char c) { return std::isdigit(c) != 0; });
}

struct LineColumn {
  size_t line = 1;
  size_t column = 1;
};

/**
 * @brief Compute the 1-based line and column for a given character offset.
 * @param code The source code to inspect.
 * @param offset The zero-based character offset in the source code.
 * @return The line and column of the offset in the source code.
 */
LineColumn lineColumnForOffset(const std::string& code, size_t offset) {
  LineColumn location;
  const auto lineStartPos = code.rfind('\n', offset);
  const size_t lineStart = (lineStartPos == std::string::npos)
                               ? 0
                               : static_cast<size_t>(lineStartPos + 1);
  location.line = 1;
  for (size_t i = 0; i < lineStart; i++) {
    if (code[i] == '\n') {
      location.line++;
    }
  }
  location.column = offset - lineStart + 1;
  return location;
}

/**
 * @brief Compute the 1-based line and column for a target within a line.
 * @param code The source code to inspect.
 * @param instructionStart The zero-based offset of the instruction start.
 * @param target The target token to locate on the line.
 * @return The line and column of the target, or the first non-space column.
 */
LineColumn lineColumnForTarget(const std::string& code, size_t instructionStart,
                               const std::string& target) {
  LineColumn location = lineColumnForOffset(code, instructionStart);
  const auto lineStartPos = code.rfind('\n', instructionStart);
  const size_t lineStart = (lineStartPos == std::string::npos)
                               ? 0
                               : static_cast<size_t>(lineStartPos + 1);
  auto lineEndPos = code.find('\n', instructionStart);
  const size_t lineEnd = (lineEndPos == std::string::npos)
                             ? code.size()
                             : static_cast<size_t>(lineEndPos);
  const auto lineText = code.substr(lineStart, lineEnd - lineStart);
  if (!target.empty()) {
    const auto targetPos = lineText.find(target);
    if (targetPos != std::string::npos) {
      location.column = targetPos + 1;
      return location;
    }
  }
  const auto nonSpace = lineText.find_first_not_of(" \t");
  if (nonSpace != std::string::npos) {
    location.column = nonSpace + 1;
  }
  return location;
}

/**
 * @brief Format a parse error with line/column location information.
 * @param code The source code to inspect.
 * @param instructionStart The zero-based offset of the instruction start.
 * @param detail The error detail text.
 * @param target Optional target token to locate more precisely.
 * @return The formatted error string.
 */
std::string formatParseError(const std::string& code, size_t instructionStart,
                             const std::string& detail,
                             const std::string& target = "") {
  const auto location = lineColumnForTarget(code, instructionStart, target);
  return "<input>:" + std::to_string(location.line) + ":" +
         std::to_string(location.column) + ": " + detail;
}

/**
 * @brief Build an error detail string for an invalid target.
 * @param target The invalid target token.
 * @param context Additional context to append.
 * @return The formatted detail string.
 */
std::string invalidTargetDetail(const std::string& target,
                                const std::string& context) {
  std::string detail = "Invalid target qubit ";
  detail += target;
  detail += context;
  detail += ".";
  return detail;
}

/**
 * @brief Build an error detail string for an invalid register declaration.
 * @param trimmedLine The register declaration line.
 * @return The formatted detail string.
 */
std::string invalidRegisterDetail(const std::string& trimmedLine) {
  std::string detail = "Invalid register declaration ";
  detail += trimmedLine;
  detail += ".";
  return detail;
}

/**
 * @brief Validate target references against known registers and indices.
 * @param code The source code to inspect.
 * @param instructionStart The zero-based offset of the instruction start.
 * @param targets The target tokens to validate.
 * @param definedRegisters The registers defined in the current scope.
 * @param shadowedRegisters The shadowed register names in the current scope.
 * @param context Additional context to append to error messages.
 */
void validateTargets(const std::string& code, size_t instructionStart,
                     const std::vector<std::string>& targets,
                     const std::map<std::string, size_t>& definedRegisters,
                     const std::vector<std::string>& shadowedRegisters,
                     const std::string& context) {
  for (const auto& target : targets) {
    if (target.empty()) {
      std::string detail = "Empty target";
      detail += context;
      detail += ".";
      throw ParsingError(formatParseError(code, instructionStart, detail));
    }
    const auto open = target.find('[');
    if (open == std::string::npos) {
      continue;
    }
    const auto close = target.find(']', open + 1);
    if (open == 0 || close == std::string::npos || close != target.size() - 1) {
      throw ParsingError(formatParseError(code, instructionStart,
                                          invalidTargetDetail(target, context),
                                          target));
    }
    const auto registerName = target.substr(0, open);
    const auto indexText = target.substr(open + 1, close - open - 1);
    if (!isDigits(indexText)) {
      throw ParsingError(formatParseError(code, instructionStart,
                                          invalidTargetDetail(target, context),
                                          target));
    }
    size_t registerIndex = 0;
    try {
      registerIndex = std::stoul(indexText);
    } catch (const std::exception&) {
      throw ParsingError(formatParseError(code, instructionStart,
                                          invalidTargetDetail(target, context),
                                          target));
    }
    if (std::ranges::find(shadowedRegisters, registerName) !=
        shadowedRegisters.end()) {
      continue;
    }
    const auto found = definedRegisters.find(registerName);
    if (found == definedRegisters.end() || found->second <= registerIndex) {
      throw ParsingError(formatParseError(code, instructionStart,
                                          invalidTargetDetail(target, context),
                                          target));
    }
  }
}

/**
 * @brief Sweep a given code string for blocks and replace them with a unique
 * identifier.
 * @param code The code to sweep.
 * @param blocks A map to store the blocks and their respective identifiers in.
 * @return The code with the blocks replaced by their identifiers.
 */
std::string sweepBlocks(const std::string& code,
                        std::map<std::string, std::string>& blocks) {
  std::string result = code;
  size_t start = 0;
  int level = 0;
  size_t pos = 0;
  while (pos < result.size()) {
    auto c = result[pos];
    if (c == '{') {
      if (level == 0) {
        start = pos;
      }
      level++;
    } else if (c == '}') {
      level--;
      if (level == 0) {
        const std::string block = result.substr(start + 1, pos - 1 - start);
        const std::string blockName =
            "$__block" + std::to_string(blocks.size()) + "$;";
        blocks[blockName] = block;
        result.replace(start, pos + 1 - start, blockName);
        pos = start;
      }
    }
    pos++;
  }
  return result;
}

/**
 * @brief Sweep a given code string for comments and replace them with
 * whitespace.
 * @param code The code to sweep.
 * @return The code with the comments replaced by whitespace.
 */
std::string removeComments(const std::string& code) {
  std::string result = code;
  for (size_t pos = 0; pos < result.size(); pos++) {
    auto nextComment = result.find("//", pos);
    if (nextComment == std::string::npos) {
      break;
    }
    auto commentEnd = result.find('\n', nextComment);
    if (commentEnd == std::string::npos) {
      commentEnd = result.size();
    }
    const std::string spaces(commentEnd - nextComment, ' ');
    result.replace(nextComment, commentEnd - nextComment, spaces);
  }
  return result;
}

/**
 * @brief Parse a function definition from a given signature.
 * @param signature The signature to parse.
 * @return The parsed function definition.
 */
FunctionDefinition parseFunctionDefinition(const std::string& signature) {
  auto parts = splitString(
      replaceString(replaceString(signature, "\n", " "), "\t", " "), ' ');
  std::string name;
  size_t index = 0;
  for (auto& part : parts) {
    index++;
    if (part != "gate" && !part.empty()) {
      name = part;
      break;
    }
  }

  std::string parameterParts;
  for (size_t i = index; i < parts.size(); i++) {
    parameterParts += parts[i];
  }
  auto parameters = splitString(removeWhitespace(parameterParts), ',');

  return {.name = name, .parameters = parameters};
}

/**
 * @brief Sweep a given code string for function names.
 * @param code The code to sweep.
 * @return A vector containing the function names.
 */
std::vector<std::string> sweepFunctionNames(const std::string& code) {
  std::vector<std::string> result;
  const std::vector<char> delimiters{';', '}'};
  const auto instructions = splitString(code, delimiters);
  for (const auto& instruction : instructions) {
    if (isFunctionDefinition(instruction)) {
      const auto f = parseFunctionDefinition(instruction);
      result.push_back(f.name);
    }
  }
  return result;
}

/**
 * @brief Unfold the targets of assertions that otherwise target full registers.
 *
 * E.g. `assert-ent q` can be unfolded into `assert-ent q[0], q[1], q[2],
 * ...`.\n `shadowedRegisters` contains all registers that are shadowed by
 * function-local variables and should therefore not be unfolded.
 * @param assertion The assertion to unfold.
 * @param definedRegisters The defined registers in the code.
 * @param shadowedRegisters The shadowed registers in the code.
 */
void unfoldAssertionTargetRegisters(
    Assertion& assertion, const std::map<std::string, size_t>& definedRegisters,
    const std::vector<std::string>& shadowedRegisters) {
  bool found = false;
  std::vector<std::string> targets;
  for (const auto& target : assertion.getTargetQubits()) {
    if (std::ranges::find(shadowedRegisters, target) !=
        shadowedRegisters.end()) {
      targets.push_back(target);
      continue;
    }
    if (definedRegisters.contains(target)) {
      for (size_t i = 0; i < definedRegisters.at(target); i++) {
        targets.push_back(target + "[" + std::to_string(i) + "]");
      }
      found = true;
    } else {
      targets.push_back(target);
    }
  }

  if (found) {
    assertion.setTargetQubits(targets);
  }
}

} // namespace

Instruction::Instruction(size_t inputLineNumber, std::string inputCode,
                         std::unique_ptr<Assertion>& inputAssertion,
                         std::vector<std::string> inputTargets, size_t startPos,
                         size_t endPos, size_t successor, bool isFuncCall,
                         std::string function, bool inFuncDef, bool isFuncDef,
                         Block inputBlock)
    : lineNumber(inputLineNumber), code(std::move(inputCode)),
      assertion(std::move(inputAssertion)), targets(std::move(inputTargets)),
      originalCodeStartPosition(startPos), originalCodeEndPosition(endPos),
      successorIndex(successor), isFunctionCall(isFuncCall),
      calledFunction(std::move(function)), inFunctionDefinition(inFuncDef),
      isFunctionDefinition(isFuncDef), block(std::move(inputBlock)) {}

bool isFunctionDefinition(const std::string& line) {
  return trim(line).starts_with("gate ");
}

bool isReset(const std::string& line) {
  return trim(line).starts_with("reset ");
}

bool isBarrier(const std::string& line) {
  return trim(line).starts_with("barrier ") ||
         trim(line).starts_with("barrier;");
}

bool isClassicControlledGate(const std::string& line) {
  return trim(line).starts_with("if") &&
         (line.find('(') != std::string::npos) &&
         (line.find(')') != std::string::npos);
}

ClassicControlledGate parseClassicControlledGate(const std::string& code) {
  std::stringstream condition;
  const auto codeSanitized = trim(replaceString(code, "if", ""));
  int openBrackets = 0;
  size_t i = 0;
  for (; i < codeSanitized.size(); i++) {
    const auto c = codeSanitized[i];
    if (c == '(') {
      openBrackets++;
    } else if (c == ')') {
      openBrackets--;
    }
    if (openBrackets == 0) {
      break;
    }
    condition << c;
  }
  auto rest = codeSanitized.substr(i + 1, codeSanitized.size() - i - 1);
  rest = replaceString(replaceString(rest, "}", ""), "{", "");
  const auto operations = splitString(rest, ';', false);
  return {.condition = condition.str(), .operations = operations};
}

bool isMeasurement(const std::string& line) {
  return line.find("->") != std::string::npos;
}

bool isVariableDeclaration(const std::string& line) {
  return trim(line).starts_with("creg ") || trim(line).starts_with("qreg ");
}

std::vector<std::string> parseParameters(const std::string& instruction) {
  if (isFunctionDefinition(instruction)) {
    const auto fd = parseFunctionDefinition(instruction);
    return fd.parameters;
  }

  if (isMeasurement(instruction)) {
    // We only add the quantum variable to the measurement's targets.
    return parseParameters(splitString(instruction, '-')[0]);
  }

  if (isClassicControlledGate(instruction)) {
    const auto classic = parseClassicControlledGate(instruction);
    std::vector<std::string> parameters;
    for (const auto& op : classic.operations) {
      const auto targets = parseParameters(op);
      parameters.insert(parameters.end(), targets.begin(), targets.end());
    }
    return parameters;
  }

  auto parts = splitString(
      replaceString(
          replaceString(replaceString(instruction, ";", " "), "\n", " "), "\t",
          " "),
      ' ');
  size_t index = 0;
  size_t openBrackets = 0;
  for (auto& part : parts) {
    index++;
    openBrackets +=
        static_cast<size_t>(std::count(part.begin(), part.end(), '('));
    openBrackets -=
        static_cast<size_t>(std::count(part.begin(), part.end(), ')'));
    if (!part.empty() && openBrackets == 0) {
      break;
    }
  }

  std::string parameterParts;
  for (size_t i = index; i < parts.size(); i++) {
    if (parts[i].empty()) {
      continue;
    }
    parameterParts += parts[i];
  }
  auto parameters = splitString(removeWhitespace(parameterParts), ',');
  if (parameters.size() == 1 && parameters[0].empty()) {
    return {};
  }
  return parameters;
}

std::vector<Instruction> preprocessCode(const std::string& code,
                                        std::string& processedCode) {
  std::map<std::string, size_t> definedRegisters;
  return preprocessCode(code, 0, 0, {}, definedRegisters, {}, processedCode);
}

std::vector<Instruction>
preprocessCode(const std::string& code, size_t startIndex,
               size_t initialCodeOffset,
               const std::vector<std::string>& allFunctionNames,
               std::map<std::string, size_t>& definedRegisters,
               const std::vector<std::string>& shadowedRegisters,
               std::string& processedCode) {

  std::map<std::string, std::string> blocks;
  std::map<std::string, size_t> functionFirstLine;
  std::map<std::string, FunctionDefinition> functionDefinitions;
  std::map<size_t, std::vector<std::string>> variableUsages;

  processedCode = removeComments(code);
  const std::string blocksRemoved = sweepBlocks(processedCode, blocks);
  std::vector<std::string> functionNames = sweepFunctionNames(processedCode);
  for (const auto& name : allFunctionNames) {
    functionNames.push_back(name);
  }

  std::vector<Instruction> instructions;

  size_t pos = 0;
  size_t i = startIndex;
  size_t blocksOffset = initialCodeOffset;

  while (pos != std::string::npos) {
    const size_t end = blocksRemoved.find(';', pos);
    if (end == std::string::npos) {
      break;
    }

    std::string line = blocksRemoved.substr(pos, end - pos + 1);
    auto trimmedLine = trim(line);
    auto tokens = splitString(trimmedLine, ' ');
    auto isAssert = isAssertion(line);
    auto blockPos = line.find("$__block");

    const auto leadingPos = blocksRemoved.find_first_not_of(" \t\r\n", pos);
    const size_t trueStart =
        ((leadingPos != std::string::npos && leadingPos < end) ? leadingPos
                                                               : pos) +
        blocksOffset;

    Block block{.valid = false, .code = ""};
    if (blockPos != std::string::npos) {
      const auto endPos = line.find('$', blockPos + 1) + 1;
      const auto blockName = line.substr(blockPos, endPos - blockPos + 1);
      const auto blockContent = blocks[blockName];

      // in the actual code, the current instruction is longer, because we
      // replaced the block with its name. Also, we add +2 because the block
      // also had `{` and `}`, which is not included in `blockContent`.
      blocksOffset += blockContent.size() + 2 - blockName.size();
      block.code = blockContent;
      block.valid = true;
      line.replace(blockPos, endPos - blockPos + 1, "");
    }

    if (block.valid && isClassicControlledGate(line)) {
      line.append(" { ").append(block.code).append(" }");
      block.valid = false;
      block.code.clear();
    }

    const auto targets = parseParameters(line);

    const size_t trueEnd = end + blocksOffset;

    if (isVariableDeclaration(line)) {
      const auto declaration = removeWhitespace(
          replaceString(replaceString(trimmedLine, "creg", ""), "qreg", ""));
      const auto parts = splitString(declaration, {'[', ']'});
      const auto& name = parts[0];
      const auto sizeText = parts.size() > 1 ? parts[1] : "";
      if (name.empty() || !isDigits(sizeText)) {
        throw ParsingError(formatParseError(
            code, trueStart, invalidRegisterDetail(trimmedLine)));
      }
      size_t size = 0;
      try {
        size = std::stoul(sizeText);
      } catch (const std::exception&) {
        throw ParsingError(formatParseError(
            code, trueStart, invalidRegisterDetail(trimmedLine)));
      }
      definedRegisters.insert({name, size});
    }

    if (isFunctionDefinition(line)) {
      if (!block.valid) {
        throw ParsingError("Gate definitions require a body block");
      }
      const auto f = parseFunctionDefinition(line);
      functionDefinitions.insert({f.name, f});
      i++;
      std::string processedSubCode;
      auto subInstructions = preprocessCode(
          block.code, i, code.find('{', trueStart) + 1, functionNames,
          definedRegisters, f.parameters, processedSubCode);
      for (auto& instr : subInstructions) {
        instr.inFunctionDefinition = true;
      }
      functionFirstLine.insert({f.name, subInstructions[0].lineNumber});
      // successor 0 means "pop call stack"
      i += subInstructions.size();

      std::unique_ptr<Assertion> a(nullptr);
      instructions.emplace_back(i - subInstructions.size() - 1, line, a,
                                targets, trueStart, trueEnd, i + 1, false, "",
                                false, true, block);
      for (auto& instr : subInstructions) {
        instructions.back().childInstructions.push_back(instr.lineNumber);
      }
      instructions.insert(instructions.end(),
                          std::make_move_iterator(subInstructions.begin()),
                          std::make_move_iterator(subInstructions.end()));

      const auto closingBrace = code.find(
          '}', instructions[instructions.size() - 1].originalCodeEndPosition);
      const Block noBlock{.valid = false, .code = ""};
      instructions.emplace_back(i, "RETURN", a, targets, closingBrace,
                                closingBrace, 0, false, "", true, false,
                                noBlock);
      i++;
      pos = end + 1;

      continue;
    }

    if (isClassicControlledGate(line)) {
      // Body blocks are handled by inlining their code into the instruction.
    }

    bool isFunctionCall = false;
    std::string calledFunction;
    if (!tokens.empty() &&
        std::ranges::find(functionNames, tokens[0]) != functionNames.end()) {
      isFunctionCall = true;
      calledFunction = tokens[0];
    }

    if (isAssert) {
      auto a = parseAssertion(line, block.code);
      unfoldAssertionTargetRegisters(*a, definedRegisters, shadowedRegisters);
      a->validate();
      validateTargets(code, trueStart, a->getTargetQubits(), definedRegisters,
                      shadowedRegisters, " in assertion");
      instructions.emplace_back(i, line, a, a->getTargetQubits(), trueStart,
                                trueEnd, i + 1, isFunctionCall, calledFunction,
                                false, false, block);
    } else {
      if (!isVariableDeclaration(line)) {
        validateTargets(code, trueStart, targets, definedRegisters,
                        shadowedRegisters, "");
      }
      std::unique_ptr<Assertion> a(nullptr);
      instructions.emplace_back(i, line, a, targets, trueStart, trueEnd, i + 1,
                                isFunctionCall, calledFunction, false, false,
                                block);

      variableUsages.insert({i, parseParameters(line)});
    }

    i++;
    pos = end + 1;
  }

  for (auto& instr : instructions) {
    auto vars = parseParameters(instr.code);
    size_t idx = instr.lineNumber - 1;
    while (instr.lineNumber != 0 && !vars.empty() &&
           (instr.lineNumber < instructions.size() ||
            idx > instr.lineNumber - instructions.size())) {
      size_t foundIndex = 0;
      for (const auto& var : variableUsages[idx]) {
        const auto found = std::ranges::find_if(
            vars, [&var](const auto& v) { return variablesEqual(v, var); });
        if (found != vars.end()) {
          std::erase(vars, var);
          instr.dataDependencies.emplace_back(idx, foundIndex);
        }
        foundIndex++;
      }
      if (idx - 1 == instr.lineNumber - instructions.size() || idx == 0) {
        break;
      }
      idx--;
    }
    if (instr.isFunctionCall) {
      instr.successorIndex = functionFirstLine[instr.calledFunction];
      if (!functionDefinitions.contains(instr.calledFunction)) {
        continue;
      }
      instr.callSubstitution.clear();
      const auto func = functionDefinitions[instr.calledFunction];
      const auto arguments = parseParameters(instr.code);
      if (func.parameters.size() != arguments.size()) {
        throw ParsingError(
            "Custom gate call uses incorrect number of arguments.");
      }
      for (size_t j = 0; j < func.parameters.size(); j++) {
        instr.callSubstitution.insert({func.parameters[j], arguments[j]});
      }
    }
  }

  return instructions;
}

} // namespace mqt::debugger
