/*
 * Copyright (c) 2024 - 2025 Chair for Design Automation, TUM
 * Copyright (c) 2025 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

/**
 * @file Utils.hpp
 * @brief Contains utility functions for parsing and string manipulation.
 */

#pragma once

#include <string>
#include <vector>

namespace mqt::debugger {

/**
 * @brief Removes leading and trailing whitespace from a string.
 * @param str The string to trim.
 * @return The trimmed string.
 */
std::string trim(const std::string& str);

/**
 * @brief Splits a string into a vector of strings based on a delimiter.
 * @param text The text to split.
 * @param delimiter The delimiter to split on.
 * @param includeEmpty Whether to include empty strings in the result.
 * @return The vector of strings.
 */
std::vector<std::string> splitString(const std::string& text, char delimiter,
                                     bool includeEmpty = true);

/**
 * @brief Splits a string into a vector of strings based on multiple delimiters.
 *
 * A split occurs when any of the delimiters are encountered.
 *
 * @param text The text to split.
 * @param delimiters The delimiters to split on.
 * @param includeEmpty Whether to include empty strings in the result.
 * @return The vector of strings.
 */
std::vector<std::string> splitString(const std::string& text,
                                     const std::vector<char>& delimiters,
                                     bool includeEmpty = true);

/**
 * @brief Replaces all occurrences of a substring in a string with another
 * string.
 * @param str The string to modify.
 * @param from The substring to replace.
 * @param to The string to replace the substring with.
 * @return The modified string.
 */
std::string replaceString(std::string str, const std::string& from,
                          const std::string& to);

/**
 * @brief Removes all whitespace from a string.
 *
 * This includes spaces, tabs, and newlines.
 *
 * @param str The string to remove whitespace from.
 * @return The string with whitespace removed.
 */
std::string removeWhitespace(std::string str);

/**
 * @brief Checks if two strings approximately refer to the same variable.
 *
 * If both strings refer to a given index of a register, they are equal if the
 * strings are exactly equal. If one string refers to a full register and the
 * other only to a single index, they are equal if they both refer to the same
 * register.
 *
 * @param v1 The first variable.
 * @param v2 The second variable.
 * @return True if the strings refer to the same variable, false otherwise.
 */
bool variablesEqual(const std::string& v1, const std::string& v2);

/**
 * @brief Extracts the base name of a register.
 *
 * @param variable The variable to extract the base name from.
 * @return The base name of the register.
 */
std::string variableBaseName(const std::string& variable);

} // namespace mqt::debugger
