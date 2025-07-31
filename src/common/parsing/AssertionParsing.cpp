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
 * @file AssertionParsing.cpp
 * @brief Implementation of the assertion parsing functionality.
 */

#include "common/parsing/AssertionParsing.hpp"

#include "common.h"
#include "common/ComplexMathematics.hpp"
#include "common/Span.hpp"
#include "common/parsing/ParsingError.hpp"
#include "common/parsing/Utils.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace mqt::debugger {

namespace {

/**
 * @brief Tests whether a qubit is in a superposition in a given statevector.
 * @param statevector The statevector to check.
 * @param qubit The index of the qubit to check.
 * @return True if the qubit is in superposition, false otherwise.
 */
bool qubitInSuperposition(const Span<Complex> statevector, size_t qubit) {
  double prob = 0;
  for (size_t i = 0; i < statevector.size(); i++) {
    if ((i & (1ULL << qubit)) != 0) {
      prob += complexMagnitude(statevector[i]);
    }
  }
  return prob > 0.00000001 && prob < 1 - 0.00000001;
}

/**
 * @brief Extract the targets of the assertion from its string representation.
 * @param targetPart The string representation of the full targets of the
 * assertion.
 * @return The targets of the assertion.
 */
std::vector<std::string> extractTargetQubits(const std::string& targetPart) {
  const auto parts = splitString(targetPart, ',');
  std::vector<std::string> trimmedParts(parts.size());
  std::ranges::transform(parts, trimmedParts.begin(), trim);
  return trimmedParts;
}

/**
 * @brief Parse a complex number from a string.
 * @param complexString The string representation of the complex number.
 * @return The parsed complex number.
 */
Complex parseComplex(std::string complexString) {
  complexString = removeWhitespace(complexString);
  auto parts = splitString(complexString, '-');
  bool negativeSplit = true;
  if (parts[0].empty()) {
    parts.erase(parts.begin());
    parts[0] = "-" + parts[0];
  }
  if (parts.size() == 1) {
    negativeSplit = false;
    parts = splitString(complexString, '+');
  }
  double real = 0;
  double imaginary = 0;
  bool first = true;
  for (auto& part : parts) {
    if (part.find('i') != std::string::npos ||
        part.find('j') != std::string::npos) {
      imaginary +=
          std::stod(replaceString(replaceString(part, "i", ""), "j", "")) *
          (negativeSplit && !first ? -1 : 1);
    } else {
      real += std::stod(part) * (negativeSplit && !first ? -1 : 1);
    }
    first = false;
  }
  return {real, imaginary};
}

/**
 * @brief Parse a statevector from a string.
 * @param statevectorString The string representation of the statevector.
 * @return The parsed statevector.
 */
Statevector parseStatevector(std::string statevectorString) {
  statevectorString = removeWhitespace(statevectorString);
  auto parts = splitString(statevectorString, ',');
  auto amplitudes = std::make_unique<std::vector<Complex>>();
  for (auto& part : parts) {
    amplitudes->push_back(parseComplex(part));
  }

  size_t numQubits = 0;
  size_t n = amplitudes->size();
  while (n > 1) {
    if ((n & 1) == 1) {
      throw ParsingError("Invalid statevector size");
    }
    n >>= 1;
    numQubits++;
  }

  Statevector sv;
  sv.numStates = amplitudes->size();
  sv.numQubits = numQubits;
  sv.amplitudes = amplitudes.release()->data();

  return sv;
}

} // namespace

Assertion::Assertion(std::vector<std::string> inputTargetQubits,
                     AssertionType assertionType)
    : targetQubits(std::move(inputTargetQubits)), type(assertionType) {}

AssertionType Assertion::getType() const { return type; }

const std::vector<std::string>& Assertion::getTargetQubits() const {
  return targetQubits;
}

void Assertion::setTargetQubits(std::vector<std::string> newTargetQubits) {
  targetQubits = std::move(newTargetQubits);
}

EntanglementAssertion::EntanglementAssertion(
    std::vector<std::string> inputTargetQubits)
    : Assertion(std::move(inputTargetQubits), AssertionType::Entanglement) {}

bool EntanglementAssertion::implies(const Assertion& other) const {
  if (other.getType() == AssertionType::Entanglement) {
    const auto containerQubits =
        std::set(getTargetQubits().begin(), getTargetQubits().end());
    const auto subQubits = std::set(other.getTargetQubits().begin(),
                                    other.getTargetQubits().end());
    return std::ranges::includes(containerQubits, subQubits);
  }
  if (other.getType() == AssertionType::Superposition) {
    return std::any_of(
        other.getTargetQubits().begin(), other.getTargetQubits().end(),
        [this](const std::string& qubit) {
          return std::find(getTargetQubits().begin(), getTargetQubits().end(),
                           qubit) != getTargetQubits().end();
        });
  }
  return false;
}

SuperpositionAssertion::SuperpositionAssertion(
    std::vector<std::string> inputTargetQubits)
    : Assertion(std::move(inputTargetQubits), AssertionType::Superposition) {}

bool SuperpositionAssertion::implies(const Assertion& other) const {
  if (other.getType() != AssertionType::Superposition) {
    // Superposition assertions can only contain other superposition assertions.
    return false;
  }
  const auto containerQubits =
      std::set(getTargetQubits().begin(), getTargetQubits().end());
  const auto subQubits =
      std::set(other.getTargetQubits().begin(), other.getTargetQubits().end());
  return std::ranges::includes(subQubits, containerQubits);
}

EqualityAssertion::EqualityAssertion(double inputSimilarityThreshold,
                                     std::vector<std::string> inputTargetQubits,
                                     AssertionType assertionType)
    : Assertion(std::move(inputTargetQubits), assertionType),
      similarityThreshold(inputSimilarityThreshold) {}

void EqualityAssertion::validate() {
  if (similarityThreshold < 0 || similarityThreshold > 1) {
    throw ParsingError("Similarity threshold must be between 0 and 1");
  }
  Assertion::validate();
}

double EqualityAssertion::getSimilarityThreshold() const {
  return similarityThreshold;
}

StatevectorEqualityAssertion::StatevectorEqualityAssertion(
    Statevector inputTargetStatevector, double inputSimilarityThreshold,
    std::vector<std::string> inputTargetQubits)
    : EqualityAssertion(inputSimilarityThreshold, std::move(inputTargetQubits),
                        AssertionType::StatevectorEquality),
      targetStatevector(inputTargetStatevector) {}

void StatevectorEqualityAssertion::validate() {
  if (targetStatevector.numQubits != getTargetQubits().size()) {
    throw ParsingError(
        "Number of target qubits must match number of qubits in statevector");
  }
  EqualityAssertion::validate();
}

const Statevector& StatevectorEqualityAssertion::getTargetStatevector() const {
  return targetStatevector;
}
StatevectorEqualityAssertion::~StatevectorEqualityAssertion() {
  delete[] targetStatevector.amplitudes;
}

bool StatevectorEqualityAssertion::implies(
    const StatevectorEqualityAssertion& other) const {
  if (getSimilarityThreshold() < other.getSimilarityThreshold()) {
    return false;
  }
  const auto containerQubits =
      std::set(getTargetQubits().begin(), getTargetQubits().end());
  const auto subQubits =
      std::set(other.getTargetQubits().begin(), other.getTargetQubits().end());
  if (!std::ranges::includes(containerQubits, subQubits)) {
    return false;
  }
  Statevector targetSV;
  std::vector<Complex> newAmplitudes;

  if (containerQubits.size() != subQubits.size()) {
    std::vector<size_t> indexList(other.getTargetQubits().size());
    std::transform(
        other.getTargetQubits().begin(), other.getTargetQubits().end(),
        indexList.begin(), [this](const std::string& target) {
          return std::distance(getTargetQubits().begin(),
                               std::find(getTargetQubits().begin(),
                                         getTargetQubits().end(), target));
        });
    newAmplitudes =
        getSubStateVectorAmplitudes(getTargetStatevector(), indexList);
    targetSV = {.numQubits = indexList.size(),
                .numStates = newAmplitudes.size(),
                .amplitudes = newAmplitudes.data()};
  } else {
    targetSV = getTargetStatevector();
  }

  const auto svSimilarity = dotProduct(targetSV, other.getTargetStatevector());
  return svSimilarity * getSimilarityThreshold() >=
         other.getSimilarityThreshold();
}

bool StatevectorEqualityAssertion::implies(
    const SuperpositionAssertion& other) const {
  const auto& targetSV = getTargetStatevector();
  const auto svSpan = Span<Complex>(targetSV.amplitudes, targetSV.numStates);
  return std::any_of(
      other.getTargetQubits().begin(), other.getTargetQubits().end(),
      [this, &svSpan](const std::string& qubit) {
        const auto found = std::find(getTargetQubits().begin(),
                                     getTargetQubits().end(), qubit);
        if (found == getTargetQubits().end()) {
          return false;
        }
        if (!qubitInSuperposition(
                svSpan, static_cast<size_t>(
                            std::distance(getTargetQubits().begin(), found)))) {
          return false;
        }
        return true;
      });
}

bool StatevectorEqualityAssertion::implies(
    const EntanglementAssertion& other) const {
  if (getSimilarityThreshold() < 0.99999) {
    // We only accept implications if similarity is high enough.
    return false;
  }
  const auto containerQubits =
      std::set(getTargetQubits().begin(), getTargetQubits().end());
  const auto subQubits =
      std::set(other.getTargetQubits().begin(), other.getTargetQubits().end());
  if (!std::ranges::includes(containerQubits, subQubits)) {
    // Only equality assertions that contain all qubits of the entanglement
    // assertion can imply it.
    return false;
  }

  std::vector<size_t> indexList(other.getTargetQubits().size());
  std::transform(other.getTargetQubits().begin(), other.getTargetQubits().end(),
                 indexList.begin(), [this](const std::string& target) {
                   return std::distance(getTargetQubits().begin(),
                                        std::find(getTargetQubits().begin(),
                                                  getTargetQubits().end(),
                                                  target));
                 });

  const Span<Complex> sv(getTargetStatevector().amplitudes,
                         getTargetStatevector().numStates);

  std::vector<std::vector<Complex>> densityMatrix(
      sv.size(), std::vector<Complex>(sv.size(), {0, 0}));
  for (size_t i = 0; i < sv.size(); i++) {
    for (size_t j = 0; j < sv.size(); j++) {
      densityMatrix[i][j] =
          complexMultiplication(sv[i], complexConjugate(sv[j]));
    }
  }

  for (const auto i : indexList) {
    for (const auto j : indexList) {
      if (i == j) {
        continue;
      }
      if (!areQubitsEntangled(densityMatrix, i, j)) {
        return false;
      }
    }
  }

  return true;
}

bool StatevectorEqualityAssertion::implies(const Assertion& other) const {
  if (other.getType() == AssertionType::StatevectorEquality) {
    const auto& svEq = dynamic_cast<const StatevectorEqualityAssertion&>(other);
    return implies(svEq);
  }
  if (other.getType() == AssertionType::Superposition) {
    const auto& sup = dynamic_cast<const SuperpositionAssertion&>(other);
    return implies(sup);
  }
  if (other.getType() == AssertionType::Entanglement) {
    const auto& ent = dynamic_cast<const EntanglementAssertion&>(other);
    return implies(ent);
  }
  return false;
}

CircuitEqualityAssertion::CircuitEqualityAssertion(
    std::string inputCircuitCode, double inputSimilarityThreshold,
    std::vector<std::string> inputTargetQubits)
    : EqualityAssertion(inputSimilarityThreshold, std::move(inputTargetQubits),
                        AssertionType::CircuitEquality),
      circuitCode(std::move(inputCircuitCode)) {}
const std::string& CircuitEqualityAssertion::getCircuitCode() const {
  return circuitCode;
}

bool CircuitEqualityAssertion::implies(const Assertion& /*other*/) const {
  return false; // `implies` method not supported for
                // CircuitEqualityAssertion");
}

/**
 * @brief Check if the given expression is an assertion.
 *
 * This is done by checking if it starts with `assert-ent`, `assert-sup`, or
 * `assert-eq`.
 * @param expression The expression to check.
 * @return True if the expression is an assertion, false otherwise.
 */
bool isAssertion(std::string expression) {
  expression = trim(expression);
  return expression.starts_with("assert-ent") ||
         expression.starts_with("assert-sup") ||
         expression.starts_with("assert-eq");
}

std::unique_ptr<Assertion> parseAssertion(std::string assertionString,
                                          const std::string& blockContent) {
  assertionString = trim(replaceString(assertionString, ";", ""));

  if (assertionString.starts_with("assert-ent")) {
    auto targets = extractTargetQubits(assertionString.substr(11));
    return std::make_unique<EntanglementAssertion>(targets);
  }
  if (assertionString.starts_with("assert-sup")) {
    auto targets = extractTargetQubits(assertionString.substr(11));
    return std::make_unique<SuperpositionAssertion>(targets);
  }
  if (assertionString.starts_with("assert-eq")) {
    auto sub = assertionString.substr(10);
    auto targets = extractTargetQubits(sub);
    double similarityThreshold = 0;
    try {
      similarityThreshold = std::stod(targets[0]);
      targets.erase(targets.begin());
    } catch (const std::invalid_argument& e) {
      similarityThreshold = 1.0;
    } catch (const std::out_of_range& e) {
      throw ParsingError(
          "Similarity threshold out of range. It must be between 0 and 1");
    }

    if (blockContent.find(';') == std::string::npos) {
      auto statevector = parseStatevector(blockContent);
      return std::make_unique<StatevectorEqualityAssertion>(
          statevector, similarityThreshold, targets);
    }
    auto circuitCode = trim(blockContent);
    return std::make_unique<CircuitEqualityAssertion>(
        circuitCode, similarityThreshold, targets);
  }
  throw ParsingError("Expression is not a valid assertion");
}

} // namespace mqt::debugger
