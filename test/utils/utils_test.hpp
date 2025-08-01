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
 * @file utils_test.cpp
 * @brief Utility functions for testing.
 */

#pragma once
#include "common.h"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace mqt::debugger::test {

/**
 * @brief Check if the given complex number is equal to the given real and
 * imaginary parts.
 *
 * Equality is tested up to an epsilon of 0.001.
 *
 * @param c The complex number to check.
 * @param real The real part to compare to.
 * @param imaginary The imaginary part to compare to.
 * @return True if the complex number is equal to the given parts, false
 * otherwise.
 */
bool complexEquality(const Complex& c, double real, double imaginary);

/**
 * @brief Check if the given variable is equal to the given boolean value.
 *
 * @param v The variable to check.
 * @param value The value to compare to.
 * @return True if the variable is equal to the given value, false otherwise.
 */
bool classicalEquals(const Variable& v, bool value);

/**
 * @brief Read the content of the `.qasm` file with the given name from the
 * `circuits` directory.
 * @param testName The name of the file to read (not including the `circuits`
 * directory path and the extension).
 * @return The content of the file as a string.
 */
std::string readFromCircuitsPath(const std::string& testName);

/**
 * @brief Generate a string representation of a complex number.
 * @param c The complex number.
 * @return The string representation of the complex number.
 */
std::string complexToStringTest(const Complex& c);

/**
 * @brief Represents a preamble entry in the compiled code.
 */
class PreambleEntry {
public:
  /**
   * @brief Get the string representation of the preamble entry.
   * @return The string representation of the preamble entry.
   */
  [[nodiscard]] virtual std::string toString() const = 0;

  virtual ~PreambleEntry() = default;
};

using PreambleVector = std::vector<std::unique_ptr<PreambleEntry>>;

/**
 * @brief A preamble entry for statistical equality assertions.
 */
class StatEqPreambleEntry : public PreambleEntry {
  /**
   * @brief The name of the variable the preamble entry is for.
   */
  std::vector<std::string> names;
  /**
   * @brief The expected ratio of |1> results for the variable's measurement or
   * another variable it is related to.
   */
  std::vector<Complex> distribution;
  /**
   * @brief The required fidelity for the variable's measurement outcomes.
   */
  double fidelity;

public:
  /**
   * @brief Constructs a new StatEqPreambleEntry with the given names,
   * distribution, and fidelity.
   * @param n The names of the variables the preamble entry is for.
   * @param dist The expected distribution of the preamble entry as complex
   * numbers.
   * @param fid The required fidelity for the preamble entry.
   */
  StatEqPreambleEntry(std::vector<std::string> n, std::vector<Complex> dist,
                      double fid)
      : names(std::move(n)), distribution(std::move(dist)), fidelity(fid) {}

  /**
   * @brief Constructs a new StatEqPreambleEntry with the given names,
   * distribution, and fidelity.
   * @param n The names of the variables the preamble entry is for.
   * @param dist The expected distribution of the preamble entry as real
   * numbers.
   * @param fid The required fidelity for the preamble entry.
   */
  StatEqPreambleEntry(std::vector<std::string> n, std::vector<double> dist,
                      double fid)
      : names(std::move(n)), distribution(dist.size()), fidelity(fid) {
    std::ranges::transform(dist, this->distribution.begin(),
                           [](double value) { return Complex{value, 0.0}; });
  }

  [[nodiscard]] std::string toString() const override {
    std::stringstream ss;
    ss << "// ASSERT: (";
    for (size_t i = 0; i < names.size(); i++) {
      ss << names[i];
      if (i < names.size() - 1) {
        ss << ",";
      }
    }
    ss << ") {";
    for (size_t i = 0; i < distribution.size(); i++) {
      ss << complexToStringTest(distribution[i]);
      if (i < distribution.size() - 1) {
        ss << ",";
      }
    }
    ss << "} " << fidelity << "\n";

    return ss.str();
  }
};

/**
 * @brief A preamble entry for statistical superposition assertions.
 */
class StatSupPreambleEntry : public PreambleEntry {
  /**
   * @brief The name of the variable the preamble entry is for.
   */
  std::vector<std::string> names;

public:
  /**
   * @brief Constructs a new StatSupPreambleEntry with the given names
   * @param n The names of the variables the preamble entry is for.
   */
  explicit StatSupPreambleEntry(std::vector<std::string> n)
      : names(std::move(n)) {}

  [[nodiscard]] std::string toString() const override {
    std::stringstream ss;
    ss << "// ASSERT: (";
    for (size_t i = 0; i < names.size(); i++) {
      ss << names[i];
      if (i < names.size() - 1) {
        ss << ",";
      }
    }
    ss << ") {superposition}\n";
    return ss.str();
  }
};

/**
 * @brief A preamble entry for projective measurement assertions.
 */
class ProjPreambleEntry : public PreambleEntry {
  /**
   * @brief The name of the variable the preamble entry is for.
   */
  std::vector<std::string> names;

public:
  /**
   * @brief Constructs a new ProjPreambleEntry with the given names
   * @param n The names of the variables the preamble entry is for.
   */
  explicit ProjPreambleEntry(std::vector<std::string> n)
      : names(std::move(n)) {}

  [[nodiscard]] std::string toString() const override {
    std::stringstream ss;
    ss << "// ASSERT: (";
    for (size_t i = 0; i < names.size(); i++) {
      ss << names[i];
      if (i < names.size() - 1) {
        ss << ",";
      }
    }
    ss << ") {zero}\n";
    return ss.str();
  }
};

} // namespace mqt::debugger::test
