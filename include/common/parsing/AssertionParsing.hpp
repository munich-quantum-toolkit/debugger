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
 * @file AssertionParsing.hpp
 * @brief Contains classes and functions for parsing and validating assertions
 * in the test files.
 */

#pragma once

#include "common.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace mqt::debugger {

/**
 * @brief Represents the type of an assertion.
 */
enum class AssertionType : uint8_t {
  /**
   * @brief Asserts that the target qubits are entangled.
   */
  Entanglement,
  /**
   * @brief Asserts that the target qubits are in superposition.
   */
  Superposition,
  /**
   * @brief Asserts that the target qubits are equal to a given statevector.
   */
  StatevectorEquality,
  /**
   * @brief Asserts that the target qubits are equal to the state produced by a
   * given circuit.
   */
  CircuitEquality
};

/**
 * @brief Represents an assertion.
 *
 * Provides a base class to be extended by the different types of assertions.
 */
class Assertion {
  /**
   * @brief The target qubits of the assertion.
   */
  std::vector<std::string> targetQubits;
  /**
   * @brief The type of the assertion.
   */
  AssertionType type;

public:
  /**
   * @brief Constructs a new Assertion with the given target qubits and type.
   * @param targetQubits The target qubits of the assertion.
   * @param type The type of the assertion.
   */
  explicit Assertion(std::vector<std::string> targetQubits, AssertionType type);

  /**
   * @brief Gets the type of the assertion.
   * @return The type of the assertion.
   */
  [[nodiscard]] AssertionType getType() const;

  /**
   * @brief Gets the target qubits of the assertion.
   * @return The target qubits of the assertion.
   */
  [[nodiscard]] const std::vector<std::string>& getTargetQubits() const;

  /**
   * @brief Destructor for the Assertion class.
   */
  virtual ~Assertion() = default;

  /**
   * @brief Sets the target qubits of the assertion.
   * @param newTargetQubits The new target qubits of the assertion.
   */
  void setTargetQubits(std::vector<std::string> newTargetQubits);

  /**
   * @brief Validates the assertion and throws an exception, if it is invalid.
   */
  virtual void validate() {};

  /**
   * @brief Checks whether this assertion also implies another given assertion.
   * @param other The other assertion to check.
   * @return True if this assertion implies the other assertion, false
   * otherwise.
   */
  [[nodiscard]] virtual bool implies(const Assertion& other) const = 0;
};

/**
 * @brief Represents an assertion that the target qubits are entangled.
 */
class EntanglementAssertion : public Assertion {
public:
  /**
   * @brief Constructs a new EntanglementAssertion with the given target qubits.
   * @param targetQubits The target qubits of the assertion.
   */
  explicit EntanglementAssertion(std::vector<std::string> targetQubits);

  /**
   * @brief Checks whether this assertion also implies another given assertion.
   * @param other The other assertion to check.
   * @return True if this assertion implies the other assertion, false
   * otherwise.
   */
  [[nodiscard]] bool implies(const Assertion& other) const override;
};

/**
 * @brief Represents an assertion that the target qubits are in superposition.
 */
class SuperpositionAssertion : public Assertion {
public:
  /**
   * @brief Constructs a new SuperpositionAssertion with the given target
   * qubits.
   * @param targetQubits The target qubits of the assertion.
   */
  explicit SuperpositionAssertion(std::vector<std::string> targetQubits);

  /**
   * @brief Checks whether this assertion also implies another given assertion.
   * @param other The other assertion to check.
   * @return True if this assertion implies the other assertion, false
   * otherwise.
   */
  [[nodiscard]] bool implies(const Assertion& other) const override;
};

/**
 * @brief Represents an assertion that the target qubits are equal to a given
 * state representation.
 */
class EqualityAssertion : public Assertion {

  /**
   * @brief The threshold for the similarity of the target qubits to the
   * statevector or circuit.
   */
  double similarityThreshold;

public:
  /**
   * @brief Constructs a new EqualityAssertion with the given similarity
   * threshold, target qubits, and type.
   * @param similarityThreshold The threshold for the similarity of the target
   * qubits to the expected state.
   * @param targetQubits The target qubits of the assertion.
   * @param type The type of the assertion.
   */
  EqualityAssertion(double similarityThreshold,
                    std::vector<std::string> targetQubits, AssertionType type);

  /**
   * @brief Gets the similarity threshold of the assertion.
   * @return The similarity threshold of the assertion.
   */
  [[nodiscard]] double getSimilarityThreshold() const;

  /**
   * @brief Validates the assertion and throws an exception, if it is invalid.
   *
   * Checks, whether the similarity threshold is inside the range of [0, 1].
   */
  void validate() override;
};

/**
 * @brief Represents an equality assertion that the target qubits are equal to a
 * given statevector.
 */
class StatevectorEqualityAssertion : public EqualityAssertion {

  /**
   * @brief The target statevector that the target qubits are compared to.
   *
   * The memory block allocated to the state vector is owned by this object.
   */
  Statevector targetStatevector;

  /**
   * @brief Checks whether this assertion also implies another given
   * StatevectorEqualityAssertion.
   * @param other The other assertion to check.
   * @return True if this assertion implies the other assertion, false
   * otherwise.
   */
  [[nodiscard]] bool implies(const StatevectorEqualityAssertion& other) const;

  /**
   * @brief Checks whether this assertion also implies another given
   * SuperpositionAssertion.
   * @param other The other assertion to check.
   * @return True if this assertion implies the other assertion, false
   * otherwise.
   */
  [[nodiscard]] bool implies(const SuperpositionAssertion& other) const;

  /**
   * @brief Checks whether this assertion also implies another given
   * EntanglementAssertion.
   * @param other The other assertion to check.
   * @return True if this assertion implies the other assertion, false
   * otherwise.
   */
  [[nodiscard]] bool implies(const EntanglementAssertion& other) const;

public:
  /**
   * @brief Constructs a new StatevectorEqualityAssertion with the given target
   * statevector, similarity threshold, and target qubits.
   * @param targetStatevector The target statevector that the target qubits are
   * compared to.
   * @param similarityThreshold The threshold for the similarity of the target
   * qubits to the expected state.
   * @param targetQubits The target qubits of the assertion.
   */
  StatevectorEqualityAssertion(Statevector targetStatevector,
                               double similarityThreshold,
                               std::vector<std::string> targetQubits);

  /**
   * @brief Gets the target statevector of the assertion.
   * @return The target statevector of the assertion.
   */
  [[nodiscard]] const Statevector& getTargetStatevector() const;

  /**
   * @brief Destructor for the StatevectorEqualityAssertion class.
   *
   * Deletes the memory block allocated to the target statevector.
   */
  ~StatevectorEqualityAssertion() override;

  /**
   * @brief Validates the assertion and throws an exception, if it is invalid.
   *
   * Checks, whether the target statevector is valid.
   */
  void validate() override;

  /**
   * @brief Checks whether this assertion also implies another given assertion.
   * @param other The other assertion to check.
   * @return True if this assertion implies the other assertion, false
   * otherwise.
   */
  [[nodiscard]] bool implies(const Assertion& other) const override;
};

/**
 * @brief Represents an equality assertion that the target qubits are equal to
 * the state produced by a given circuit.
 */
class CircuitEqualityAssertion : public EqualityAssertion {

  /**
   * @brief The code of the circuit that the target qubits are compared to.
   */
  std::string circuitCode;

public:
  /**
   * @brief Constructs a new CircuitEqualityAssertion with the given circuit
   * code, similarity threshold, and target qubits.
   * @param circuitCode The code of the circuit that the target qubits are
   * compared to.
   * @param similarityThreshold The threshold for the similarity of the target
   * qubits to the expected state.
   * @param targetQubits The target qubits of the assertion.
   */
  CircuitEqualityAssertion(std::string circuitCode, double similarityThreshold,
                           std::vector<std::string> targetQubits);

  /**
   * @brief Gets the code of the circuit that the target qubits are compared to.
   * @return The code of the circuit that the target qubits are compared to.
   */
  [[nodiscard]] const std::string& getCircuitCode() const;

  /**
   * @brief Checks whether this assertion also implies another given assertion.
   * @param other The other assertion to check.
   * @return True if this assertion implies the other assertion, false
   * otherwise.
   */
  [[nodiscard]] bool implies(const Assertion& other) const override;
};

/**
 * @brief Parses an assertion from the given string and block content.
 *
 * The `blockContent` string may be empty if an entanglement assertion or a
 * superposition assertion is parsed.
 *
 * @param assertionString The string representation of the assertion.
 * @param blockContent The content of the block containing the assertion body.
 * @return The parsed assertion.
 */
std::unique_ptr<Assertion> parseAssertion(std::string assertionString,
                                          const std::string& blockContent);

/**
 * @brief Checks whether the given string represents an assertion.
 *
 * For that, it has to start with the keywords "assert-eq", "assert-ent", or
 * "assert-sup".
 * @param expression The string to check.
 * @return True, if the string represents an assertion, false otherwise.
 */
bool isAssertion(std::string expression);

} // namespace mqt::debugger
