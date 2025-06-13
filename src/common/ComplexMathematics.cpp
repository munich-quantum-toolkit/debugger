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
 * @file ComplexMathematics.cpp
 * @brief Implementation of maths methods for complex numbers.
 */

#include "common/ComplexMathematics.hpp"

#include "Eigen/src/Eigenvalues/ComplexEigenSolver.h"
#include "common.h"
#include "common/Span.hpp"

#include <Eigen/Dense>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace mqt::debugger {

/**
 * @brief Compute the trace of the square of a given matrix.
 *
 * This is done so that we don't have to compute the entire square of the
 * matrix.
 * @param matrix The matrix to compute the trace of the square of.
 * @return The computed trace of the square.
 */
Complex getTraceOfSquare(const std::vector<std::vector<Complex>>& matrix) {
  Complex runningSum{0, 0};
  for (size_t i = 0; i < matrix.size(); i++) {
    for (size_t k = 0; k < matrix.size(); k++) {
      runningSum = complexAddition(
          runningSum, complexMultiplication(matrix[i][k], matrix[k][i]));
    }
  }
  return runningSum;
}

/**
 * @brief Split a given number into two numbers partitioned by the bits at the
 * given indices.
 *
 * Starts by extracting the bits at the given indices of its binary
 * representation. Then, the extracted and non-extracted parts are converted
 * into two new numbers.
 * @param number The number to split.
 * @param n The number of bits in the binary representation.
 * @param bits The indices of the bits to extract.
 * @return The two new numbers, partitioned by the extracted bits.
 */
std::pair<size_t, size_t> splitBitString(size_t number, size_t n,
                                         const std::vector<size_t>& bits) {
  size_t lenFirst = 0;
  size_t lenSecond = 0;

  size_t first = 0;
  size_t second = 0;

  for (size_t index = 0; index < n; index++) {
    if (std::find(bits.begin(), bits.end(), index) != bits.end()) {
      first |= (number & 1) << lenFirst;
      lenFirst++;
    } else {
      second |= (number & 1) << lenSecond;
      lenSecond++;
    }
    number >>= 1;
  }
  return {first, second};
}

/**
 * @brief Compute the partial trace of a given matrix.
 * @param matrix The matrix to compute the partial trace of.
 * @param indicesToTraceOut The indices of the qubits to trace out.
 * @param nQubits The total number of qubits.
 * @return The computed partial trace.
 */
std::vector<std::vector<Complex>>
getPartialTrace(const std::vector<std::vector<Complex>>& matrix,
                const std::vector<size_t>& indicesToTraceOut, size_t nQubits) {
  const auto traceSize = 1ULL << (nQubits - indicesToTraceOut.size());
  std::vector<std::vector<Complex>> traceMatrix(
      traceSize, std::vector<Complex>(traceSize, {0, 0}));
  for (size_t i = 0; i < matrix.size(); i++) {
    for (size_t j = 0; j < matrix.size(); j++) {
      const auto split1 = splitBitString(i, nQubits, indicesToTraceOut);
      const auto split2 = splitBitString(j, nQubits, indicesToTraceOut);
      if (split1.first != split2.first) {
        continue;
      }
      traceMatrix[split1.second][split2.second] = complexAddition(
          traceMatrix[split1.second][split2.second], matrix[i][j]);
    }
  }
  return traceMatrix;
}

/**
 * @brief Compute the entropy of a given matrix.
 * @param matrix The matrix to compute the entropy of.
 * @return The computed entropy.
 */
double getEntropy(const std::vector<std::vector<Complex>>& matrix) {
  const auto mat = toEigenMatrix(matrix);

  const Eigen::ComplexEigenSolver<Eigen::MatrixXcd> solver(mat); // NOLINT
  const auto& eigenvalues = solver.eigenvalues();
  double entropy = 0;
  for (const auto val : eigenvalues) {
    const auto value =
        (val.real() > -0.00001 && val.real() < 0) ? 0 : val.real();
    if (value < 0) {
      throw std::runtime_error("Negative eigenvalue");
    }
    if (value != 0) {
      entropy -= val.real() * log2(val.real());
    }
  }
  return entropy;
}

/**
 * @brief Compute the shared information of a given 4x4 density matrix.
 *
 * The density matrix is assumed to represent a two-qubit system.\n
 * The shared information is computed by adding up the entropy values of the two
 * new matrices created by tracing out each of the qubits and subtracting the
 * entropy of the original matrix.
 * @param matrix The density matrix to compute the shared information of.
 * @return The computed shared information.
 */
double getSharedInformation(const std::vector<std::vector<Complex>>& matrix) {
  const auto p0 = getPartialTrace(matrix, {1}, 2);
  const auto p1 = getPartialTrace(matrix, {0}, 2);
  return getEntropy(p0) + getEntropy(p1) - getEntropy(matrix);
}

Complex complexAddition(const Complex& c1, const Complex& c2) {
  const double real = c1.real + c2.real;
  const double imaginary = c1.imaginary + c2.imaginary;
  return {real, imaginary};
}

Complex complexMultiplication(const Complex& c1, const Complex& c2) {
  const double real = (c1.real * c2.real) - (c1.imaginary * c2.imaginary);
  const double imaginary = (c1.real * c2.imaginary) + (c1.imaginary * c2.real);
  return {real, imaginary};
}

Complex complexConjugate(const Complex& c) { return {c.real, -c.imaginary}; }

bool areQubitsEntangled(std::vector<std::vector<Complex>>& densityMatrix,
                        size_t qubit1, size_t qubit2) {
  const auto numQubits = static_cast<size_t>(std::log2(densityMatrix.size()));
  if (numQubits == 2) {
    return getSharedInformation(densityMatrix) > 0;
  }
  std::vector<size_t> toTraceOut;
  for (size_t i = 0; i < numQubits; i++) {
    if (i != qubit1 && i != qubit2) {
      toTraceOut.push_back(i);
    }
  }
  const auto partialTrace =
      getPartialTrace(densityMatrix, toTraceOut, numQubits);
  return getSharedInformation(partialTrace) > 0;
}

std::vector<std::vector<Complex>> toDensityMatrix(const Statevector& sv) {
  const Span<Complex> amplitudes(sv.amplitudes, sv.numStates);
  std::vector<std::vector<Complex>> densityMatrix(
      sv.numStates, std::vector<Complex>(sv.numStates, {0, 0}));
  for (size_t i = 0; i < sv.numStates; i++) {
    for (size_t j = 0; j < sv.numStates; j++) {
      densityMatrix[i][j] =
          complexMultiplication(amplitudes[i], complexConjugate(amplitudes[j]));
    }
  }

  return densityMatrix;
}

bool partialTraceIsPure(const Statevector& sv,
                        const std::vector<size_t>& traceOut) {
  const auto traceMatrix = getPartialTraceFromStateVector(sv, traceOut);
  const auto trace = getTraceOfSquare(traceMatrix);
  const double epsilon = 0.0001;
  return trace.imaginary < epsilon && trace.imaginary > -epsilon &&
         (trace.real - 1) < epsilon && (trace.real - 1) > -epsilon;
}

std::vector<std::vector<Complex>>
getPartialTraceFromStateVector(const Statevector& sv,
                               const std::vector<size_t>& traceOut) {
  const auto traceSize = 1ULL << (sv.numQubits - traceOut.size());
  const Span<Complex> amplitudes(sv.amplitudes, sv.numStates);
  std::vector<std::vector<Complex>> traceMatrix(
      traceSize, std::vector<Complex>(traceSize, {0, 0}));
  for (size_t i = 0; i < sv.numStates; i++) {
    for (size_t j = 0; j < sv.numStates; j++) {
      const auto split1 = splitBitString(i, sv.numQubits, traceOut);
      const auto split2 = splitBitString(j, sv.numQubits, traceOut);
      if (split1.first != split2.first) {
        continue;
      }
      const auto product = complexMultiplication(amplitudes[i], amplitudes[j]);
      const auto row = split1.second;
      const auto col = split2.second;
      traceMatrix[row][col] = complexAddition(traceMatrix[row][col], product);
    }
  }
  return traceMatrix;
}

Eigen::MatrixXcd // NOLINT
toEigenMatrix(const std::vector<std::vector<Complex>>& matrix) {
  Eigen::MatrixXcd mat(static_cast<int64_t>(matrix.size()),  // NOLINT
                       static_cast<int64_t>(matrix.size())); // NOLINT
  for (size_t i = 0; i < matrix.size(); i++) {
    for (size_t j = 0; j < matrix.size(); j++) {
      mat(static_cast<int64_t>(i), static_cast<int64_t>(j)) = {
          matrix[i][j].real, matrix[i][j].imaginary};
    }
  }
  return mat;
}

double complexMagnitude(Complex& c) {
  return std::sqrt((c.real * c.real) + (c.imaginary * c.imaginary));
}

std::vector<Complex>
getSubStateVectorAmplitudes(const Statevector& sv,
                            const std::vector<size_t>& qubits) {
  std::vector<size_t> otherQubits;
  for (size_t i = 0; i < sv.numQubits; i++) {
    if (std::find(qubits.begin(), qubits.end(), i) == qubits.end()) {
      otherQubits.push_back(i);
    }
  }

  const auto traced = getPartialTraceFromStateVector(sv, otherQubits);

  // Create Eigen3 Matrix
  const auto mat = toEigenMatrix(traced);

  const Eigen::ComplexEigenSolver<Eigen::MatrixXcd> solver(mat); // NOLINT

  const auto& vectors = solver.eigenvectors();
  const auto& values = solver.eigenvalues();
  const auto epsilon = 0.0001;
  int index = -1;
  for (int i = 0; i < values.size(); i++) {
    if (values[i].imag() < -epsilon || values[i].imag() > epsilon) {
      continue;
    }
    if (values[i].real() - 1 < -epsilon || values[i].real() - 1 > epsilon) {
      continue;
    }
    index = i;
  }

  if (index == -1) {
    throw std::runtime_error("No valid index found");
  }

  std::vector<Complex> amplitudes;

  for (size_t i = 0; i < traced.size(); i++) {
    amplitudes.push_back({vectors(static_cast<int>(i), index).real(),
                          vectors(static_cast<int>(i), index).imag()});
  }
  return amplitudes;
}

/**
 * @brief Generate a string representation of a double without trailing zeros.
 *
 * @param d The double to convert to a string.
 * @return The string representation of the double.
 */
std::string doubleToString(const double d) {
  auto string = std::to_string(d);
  while (string.back() == '0') {
    string.pop_back();
  }
  if (string.back() == '.') {
    string.pop_back();
  }
  return string;
}

std::string complexToString(const Complex& c) {
  const double epsilon = 0.0000001;
  if (c.imaginary < epsilon && c.imaginary > -epsilon) {
    return doubleToString(c.real);
  }
  if (c.real < epsilon && c.real > -epsilon) {
    return doubleToString(c.imaginary) + "i";
  }
  return doubleToString(c.real) + " + " + doubleToString(c.imaginary) + "i";
}

double dotProduct(const Statevector& sv1, const Statevector& sv2) {
  double resultReal = 0;
  double resultImag = 0;

  const Span<Complex> amplitudes1(sv1.amplitudes, sv1.numStates);
  const Span<Complex> amplitudes2(sv2.amplitudes, sv2.numStates);

  for (size_t i = 0; i < sv1.numStates; i++) {
    resultReal += amplitudes1[i].real * amplitudes2[i].real +
                  amplitudes1[i].imaginary * amplitudes2[i].imaginary;
    resultImag += -1 * amplitudes1[i].real * amplitudes2[i].imaginary +
                  amplitudes1[i].imaginary * amplitudes2[i].real;
  }
  Complex result{resultReal, resultImag};
  return complexMagnitude(result);
}

} // namespace mqt::debugger
