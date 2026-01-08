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
 * @file ComplexMathematics.hpp
 * @brief Provides maths methods for complex numbers.
 */

#pragma once

#include "common.h"

#include <Eigen/Dense>
#include <cstddef>
#include <string>
#include <vector>

namespace mqt::debugger {

/**
 * @brief Compute the magnitude of a complex number.
 * @param c The complex number.
 * @return The computed magnitude.
 */
double complexMagnitude(Complex& c);

/**
 * @brief Add two complex numbers.
 * @param c1 The first complex number.
 * @param c2 The second complex number.
 * @return The sum of the two complex numbers.
 */
Complex complexAddition(const Complex& c1, const Complex& c2);

/**
 * @brief Multiply two complex numbers.
 * @param c1 The first complex number.
 * @param c2 The second complex number.
 * @return The product of the two complex numbers.
 */
Complex complexMultiplication(const Complex& c1, const Complex& c2);

/**
 * @brief Compute the complex conjugate of a complex number.
 * @param c The complex number.
 * @return The complex conjugate of the input complex number.
 */
Complex complexConjugate(const Complex& c);

/**
 * @brief Check if two qubits are entangled in a given density matrix.
 *
 * This is done by tracing out all other qubits from a density matrix and then
 * checking whether the shared information is greater than 0.
 * @param densityMatrix The density matrix to check for entanglement.
 * @param qubit1 The first qubit to check.
 * @param qubit2 The second qubit to check.
 * @return True if the qubits are entangled, false otherwise.
 */
bool areQubitsEntangled(std::vector<std::vector<Complex>>& densityMatrix,
                        size_t qubit1, size_t qubit2);

/**
 * @brief Translate a given statevector to a density matrix.
 *
 * @param sv The statevector to translate.
 * @return The computed density matrix.
 */
std::vector<std::vector<Complex>> toDensityMatrix(const Statevector& sv);

/**
 * @brief Check if the partial trace of a given state vector is pure.
 *
 * This is true if and only if the trace of the square of the traced out density
 * matrix is equal to 1.
 * @param sv The state vector to check.
 * @param traceOut The indices of the qubits to trace out.
 * @return True if the partial trace is pure, false otherwise.
 */
bool partialTraceIsPure(const Statevector& sv,
                        const std::vector<size_t>& traceOut);

/**
 * @brief Gets the partial state vector by tracing out individual qubits from
 * the full state vector.
 * @param sv The full state vector.
 * @param traceOut The indices of the qubits to trace out.
 * @return The partial state vector.
 */
std::vector<std::vector<Complex>>
getPartialTraceFromStateVector(const Statevector& sv,
                               const std::vector<size_t>& traceOut);

/**
 * @brief Convert a given vector-of-vectors matrix to an Eigen3 matrix.
 * @param matrix The vector-of-vectors matrix to convert.
 * @return A new Eigen3 matrix.
 */
Eigen::MatrixXcd // NOLINT
toEigenMatrix(const std::vector<std::vector<Complex>>& matrix);

/**
 * @brief Compute the amplitudes of a given state vector's sub-state.
 * @param sv The state vector to compute the sub-state from.
 * @param qubits The indices of the qubits to include in the sub-state.
 * @return The computed sub-state vector amplitudes.
 */
std::vector<Complex>
getSubStateVectorAmplitudes(const Statevector& sv,
                            const std::vector<size_t>& qubits);

/**
 * @brief Generate a string representation of a complex number.
 * @param c The complex number.
 * @return The string representation of the complex number.
 */
std::string complexToString(const Complex& c);

/**
 * @brief Compute the dot product of two state vectors.
 * @param sv1 The first state vector.
 * @param sv2 The second state vector.
 * @return The computed dot product.
 */
double dotProduct(const Statevector& sv1, const Statevector& sv2);

} // namespace mqt::debugger
