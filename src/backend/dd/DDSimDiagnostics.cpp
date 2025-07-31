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
 * @file DDSimDiagnostics.cpp
 * @brief Implementation of the diagnostics interface for the DD simulator.
 */

#include "backend/dd/DDSimDiagnostics.hpp"

#include "backend/dd/DDSimDebug.hpp"
#include "backend/diagnostics.h"
#include "common.h"
#include "common/ComplexMathematics.hpp"
#include "common/Span.hpp"
#include "common/parsing/AssertionParsing.hpp"
#include "common/parsing/AssertionTools.hpp"
#include "common/parsing/CodePreprocessing.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace mqt::debugger {

namespace {

/**
 * @brief Cast a `Diagnostics` pointer to a `DDDiagnostics` pointer.
 *
 * This was defined as a function to avoid linting issues due to the frequent
 * `reinterpret_cast`s.
 * @param diagnostics The `Diagnostics` pointer to cast.
 * @return The `DDDiagnostics` pointer.
 */
DDDiagnostics* toDDDiagnostics(Diagnostics* diagnostics) {
  // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
  return reinterpret_cast<DDDiagnostics*>(diagnostics);
  // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
}

/**
 * @brief Cast a `uint8_t` pointer to a `bool` pointer.
 *
 * This was defined as a function to avoid linting issues due to the frequent
 * `reinterpret_cast`s.
 * @param array The `uint8_t` pointer to cast.
 * @return The `bool` pointer.
 */
bool* toBoolArray(uint8_t* array) {
  // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
  return reinterpret_cast<bool*>(array);
  // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
}

/**
 * @brief Get the return instruction of the given instruction's scope.
 *
 * This should only be executed if we know a return instruction exists.
 * @param state The simulation state.
 * @param instruction The instruction to start the search from.
 * @return The index of the return instruction.
 */
size_t findReturn(DDSimulationState* state, size_t instruction) {
  size_t current = instruction;
  while (state->instructionTypes[current] != RETURN) {
    current++;
  }
  return current;
}

/**
 * @brief Iterate over a function call when computing data dependencies.
 *
 * When a function call is added to the `toVisit` set during the search for data
 * dependencies, this function also tries to find possible dependencies inside
 * the called function. This can lead to recursive calls of this function.
 * @param ddsim The simulation state.
 * @param current The index of the call for which to find dependencies.
 * @param qubitIndex The index of the qubit in the call's argument list.
 * @param visited The set of already visited instructions.
 * @param toVisit The set of instructions to visit next.
 */
void visitCall(DDSimulationState* ddsim, size_t current, size_t qubitIndex,
               std::set<size_t>& visited, std::set<size_t>& toVisit) {
  const auto gateStart = ddsim->successorInstructions[current];
  const auto gateDefinition = gateStart - 1;
  const std::string stringToSearch =
      ddsim->targetQubits[gateDefinition][qubitIndex];
  auto checkInstruction = findReturn(ddsim, gateStart);
  while (checkInstruction >= gateStart) {
    const auto found =
        std::find(ddsim->targetQubits[checkInstruction].begin(),
                  ddsim->targetQubits[checkInstruction].end(), stringToSearch);
    if (ddsim->instructionTypes[checkInstruction] != RETURN &&
        found != ddsim->targetQubits[checkInstruction].end()) {
      if (!visited.contains(checkInstruction)) {
        toVisit.insert(checkInstruction);
      }
      if (ddsim->instructionTypes[checkInstruction] == CALL) {
        const auto position =
            std::distance(ddsim->targetQubits[checkInstruction].begin(), found);
        visitCall(ddsim, checkInstruction, static_cast<size_t>(position),
                  visited, toVisit);
      }
      break;
    }
    if (checkInstruction == 0) {
      break;
    }
    checkInstruction--;
  }
}

/**
 * @brief Get the set of custom gate definitions for which the caller is
 * unknown.
 *
 * This is used when searching data dependencies with the `includeCallers` flag
 * set to `true`.
 * @param ddsim The simulation state.
 * @param instruction The index of the function definition to search the callers
 * for.
 * @return A set of function definitions whose callers are unknown.
 */
std::set<size_t> getUnknownCallers(DDSimulationState* ddsim,
                                   size_t instruction) {
  std::set<size_t> unknownCallers;

  std::set<size_t> toVisit{};
  std::set<size_t> visited{};

  while (true) {
    instruction--;
    if (ddsim->functionDefinitions.contains(instruction)) {
      unknownCallers.insert(instruction);
      for (const auto caller : ddsim->functionCallers[instruction]) {
        if (!visited.contains(caller)) {
          toVisit.insert(caller);
        }
      }
    }

    if (instruction == 0 || ddsim->instructionTypes[instruction] == RETURN ||
        ddsim->functionDefinitions.contains(instruction)) {
      if (toVisit.empty()) {
        break;
      }

      instruction = *toVisit.begin();
      toVisit.erase(instruction);
      visited.insert(instruction);
    }
  }

  return unknownCallers;
}

/**
 * @brief Get the interactions tree of the given qubit at runtime.
 *
 * At runtime, we can store precisely what qubits are used by each instruction,
 * so we have more specific details than at static time.
 * @param ddd The dd diagnostics instance.
 * @param qubit The qubit to find the interaction tree for.
 * @return A set of edges between qubits on the interaction tree and the
 * instruction they were taken from.
 */
std::set<std::tuple<size_t, size_t, size_t>>
getInteractionTreeAtRuntime(DDDiagnostics* ddd, size_t qubit) {
  auto* ddsim = ddd->simulationState;
  std::set<size_t> interactions;
  std::set<std::tuple<size_t, size_t, size_t>> tree;
  interactions.insert(qubit);
  bool found = true;

  while (found) {
    found = false;

    for (size_t i = 0; i < ddsim->instructionTypes.size(); i++) {
      if (ddsim->instructionTypes[i] != SIMULATE) {
        continue;
      }
      if (!ddd->actualQubits.contains(i)) {
        continue;
      }

      auto& actualQubits = ddd->actualQubits[i];
      for (const auto& actualQubitVector : actualQubits) {
        if (!std::ranges::none_of(actualQubitVector,
                                  [&interactions](size_t elem) {
                                    return interactions.contains(elem);
                                  })) {
          for (size_t target1 = 0; target1 < actualQubitVector.size();
               target1++) {
            const auto& target = actualQubitVector[target1];
            if (!interactions.contains(target)) {
              found = true;
            }
            for (size_t target2 = 1; target2 < actualQubitVector.size();
                 target2++) {
              const auto& secondTarget = actualQubitVector[target2];
              tree.insert({target, secondTarget, i});
              tree.insert({secondTarget, target, i});

              if (!interactions.contains(secondTarget)) {
                found = true;
              }
              interactions.insert(secondTarget);
            }
            interactions.insert(target);
          }
        }
      }
    }
  }

  return tree;
}

/**
 * @brief Get interactions of the given qubit at runtime.
 *
 * At runtime, we can store precisely what qubits are used by each instruction,
 * so we have more specific details than at static time.
 * @param ddd The dd diagnostics instance.
 * @param qubit The qubit to find interactions for.
 * @return A set of qubits that interact with the given qubit.
 */
std::set<size_t> getInteractionsAtRuntime(DDDiagnostics* ddd, size_t qubit) {
  const auto tree = getInteractionTreeAtRuntime(ddd, qubit);
  std::set<size_t> interactions{qubit};
  for (const auto& pair : tree) {
    interactions.insert(std::get<0>(pair));
    interactions.insert(std::get<1>(pair));
  }
  return interactions;
}

/**
 * @brief Check if the given qubit is always zero in the given statevector.
 *
 * If the `checkOne` flag is set to `true`, the function checks if the qubit is
 * always one instead.
 * @param sv The statevector to check.
 * @param qubit The qubit to check.
 * @param checkOne If true, the function checks if the qubit is always one.
 * @return True if the qubit is always zero (or one), false otherwise.
 */
bool isAlwaysZero(const Statevector& sv, size_t qubit, bool checkOne = false) {
  const auto epsilon = 1e-10;

  const Span<Complex> amplitudes(sv.amplitudes, sv.numStates);

  for (size_t i = 0; i < sv.numStates; i++) {
    if (((i & (1ULL << qubit)) == 0ULL && !checkOne) ||
        ((i & (1ULL << qubit)) != 0ULL && checkOne)) {
      continue;
    }
    if (amplitudes[i].real > epsilon || amplitudes[i].real < -epsilon ||
        amplitudes[i].imaginary > epsilon ||
        amplitudes[i].imaginary < -epsilon) {
      return false;
    }
  }

  return true;
}

/**
 * @brief Finds a unique path between two qubits in the given graph, if it
 * exists.
 *
 * If no such path exists, returns an empty vector.
 *
 * @param graph The graph to search in as a set of triples (start, end,
 * instruction).
 * @param start The starting vertex.
 * @param end The ending vertex.
 * @return The unique path between the two vertices, if it exists.
 */
std::vector<std::tuple<size_t, size_t, size_t>>
findUniquePath(const std::set<std::tuple<size_t, size_t, size_t>>& graph,
               size_t start, size_t end) {
  std::set<size_t> visited;
  std::vector<size_t> toVisit;

  std::map<size_t, std::pair<size_t, size_t>> predecessors;
  std::set<size_t> multiplePredecessors;

  toVisit.push_back(start);
  while (!toVisit.empty()) {
    auto current = *toVisit.begin();
    toVisit.erase(toVisit.begin());
    visited.insert(current);
    if (current == end) {
      break;
    }

    for (const auto& edge : graph) {
      size_t other = 0;
      const size_t instruction = std::get<2>(edge);
      if (std::get<0>(edge) == current) {
        other = std::get<1>(edge);
      } else if (std::get<1>(edge) == current) {
        other = std::get<0>(edge);
      } else {
        continue;
      }
      if (other == current) {
        // Fail-safe in case this ever comes up.
        continue;
      }

      if (predecessors.contains(current) &&
          predecessors[current].first == other &&
          predecessors[current].second == instruction) {
        // This is the edge we came from, so we don't want to go back.
        continue;
      }

      if (predecessors.contains(other)) {
        if (predecessors[other].second != instruction) {
          multiplePredecessors.insert(other);
        }
      } else {
        predecessors.insert({other, {current, instruction}});
      }
      if (visited.contains(other)) {
        continue;
      }
      if (std::ranges::find(toVisit, other) == toVisit.end()) {
        toVisit.push_back(other);
      }
    }
  }

  if (!predecessors.contains(end)) {
    return {};
  }
  std::vector<std::tuple<size_t, size_t, size_t>> path;
  size_t current = end;
  while (current != start) {
    if (multiplePredecessors.contains(current)) {
      return {};
    }
    path.emplace_back(predecessors[current].first, current,
                      predecessors[current].second);
    current = predecessors[current].first;
  }
  return path;
}

/**
 * @brief Suggest new assertions based on a failed entanglement assertion.
 * @param self The diagnostics instance.
 * @param instructionIndex The index of the assertion that failed.
 * @param assertion The assertion that failed.
 */
void suggestBasedOnFailedEntanglementAssertion(
    DDDiagnostics* self, size_t instructionIndex,
    const EntanglementAssertion* assertion) {
  // For larger assertions, first split it into smaller ones.
  if (assertion->getTargetQubits().size() != 2) {
    if (!self->assertionsEntToInsert.contains(instructionIndex)) {
      self->assertionsEntToInsert.insert(
          {instructionIndex,
           std::set<std::pair<std::set<std::string>, size_t>>()});
    }
    for (size_t i = 0; i < assertion->getTargetQubits().size(); i++) {
      const auto qubit = assertion->getTargetQubits()[i];
      for (size_t j = i + 1; j < assertion->getTargetQubits().size(); j++) {
        const auto other = assertion->getTargetQubits()[j];
        self->assertionsEntToInsert[instructionIndex].insert(
            {{qubit, other}, instructionIndex});
      }
    }
    return;
  }

  const auto& actualQubitVector = self->actualQubits[instructionIndex];
  std::set<std::tuple<size_t, size_t, size_t>> generalInteractions;
  bool first = true;
  for (const auto& actualQubits : actualQubitVector) {
    std::set<std::tuple<size_t, size_t, size_t>> addingInteractions;
    const auto interactionGraph =
        getInteractionTreeAtRuntime(self, actualQubits[1]);
    const auto baseQubit = actualQubits[0];
    const auto targetQubit = actualQubits[1];

    const auto path = findUniquePath(interactionGraph, baseQubit, targetQubit);

    for (const auto& edge : path) {
      const auto from = std::get<0>(edge);
      const auto to = std::get<1>(edge);
      const auto trueFrom = from > to ? to : from;
      const auto trueTo = from > to ? from : to;
      addingInteractions.insert({trueFrom, trueTo, std::get<2>(edge)});
    }

    if (first) {
      generalInteractions = addingInteractions;
      first = false;
    } else {
      std::set<std::tuple<size_t, size_t, size_t>> newInteractions;
      std::ranges::set_intersection(
          generalInteractions, addingInteractions,
          std::inserter(newInteractions, newInteractions.begin()));
      generalInteractions = newInteractions;
      if (generalInteractions.empty()) {
        return;
      }
    }
  }

  if (!self->assertionsEntToInsert.contains(instructionIndex)) {
    self->assertionsEntToInsert.insert(
        {instructionIndex,
         std::set<std::pair<std::set<std::string>, size_t>>()});
  }
  for (const auto& entry : generalInteractions) {
    const auto q1 = std::get<0>(entry);
    const auto q2 = std::get<1>(entry);
    auto* ddsim = self->simulationState;
    self->assertionsEntToInsert[instructionIndex].insert(
        {{getQuantumBitName(ddsim, q1), getQuantumBitName(ddsim, q2)},
         std::get<2>(entry) +
             1}); // + 1 because the assertion lands after the instruction.
  }
}

/**
 * @brief Suggest new assertions based on the splitting of an equality
 * assertion.
 * @param self The diagnostics instance.
 * @param instructionIndex The index of the assertion to split.
 * @param assertion The assertion to split.
 */
void suggestSplitEqualityAssertion(
    DDDiagnostics* self, size_t instructionIndex,
    const StatevectorEqualityAssertion* assertion) {
  const auto& sv = assertion->getTargetStatevector();

  const auto densityMatrix = toDensityMatrix(sv);

  std::vector<size_t> separableQubits;
  for (size_t i = 0; i < sv.numQubits; i++) {
    if (i == sv.numQubits - 1 && separableQubits.size() == i) {
      // Leave at least one element in remaining.
      break;
    }
    if (partialTraceIsPure(sv, {i})) {
      separableQubits.push_back(i);
    }
  }

  if (separableQubits.empty()) {
    return;
  }

  std::vector<size_t> remainingQubits;
  for (size_t i = 0; i < sv.numQubits; i++) {
    if (std::ranges::find(separableQubits, i) == separableQubits.end()) {
      remainingQubits.push_back(i);
    }
  }

  std::vector<std::vector<Complex>> extractedAmplitudes;
  std::vector<std::vector<std::string>> targetQubits;
  for (const size_t qb : separableQubits) {
    extractedAmplitudes.push_back(getSubStateVectorAmplitudes(sv, {qb}));
    targetQubits.push_back({assertion->getTargetQubits()[qb]});
  }
  extractedAmplitudes.push_back(
      getSubStateVectorAmplitudes(sv, remainingQubits));
  std::vector<std::string> remainingQubitNames;
  std::ranges::transform(
      remainingQubits, std::back_inserter(remainingQubitNames),
      [&assertion](size_t qb) { return assertion->getTargetQubits()[qb]; });
  targetQubits.push_back(remainingQubitNames);

  const auto similarity = assertion->getSimilarityThreshold();

  for (size_t i = 0; i < extractedAmplitudes.size(); i++) {

    const auto& amplitudeSet = extractedAmplitudes[i];
    const auto& targetQubitSet = targetQubits[i];
    auto toInsert =
        InsertEqualityAssertion{.instructionIndex = instructionIndex,
                                .amplitudes = {},
                                .similarity = similarity,
                                .targets = targetQubitSet};

    // Round amplitudes if necessary.
    const auto roundingFactor = 1e8;
    std::ranges::transform(
        amplitudeSet, std::back_inserter(toInsert.amplitudes),
        [&roundingFactor](const Complex& c) {
          return Complex{
              .real = std::round(c.real * roundingFactor) / roundingFactor,
              .imaginary =
                  std::round(c.imaginary * roundingFactor) / roundingFactor};
        });
    // If an amplitude was rounded, we adapt the similarity if it is too high
    // otherwise.
    for (size_t j = 0; j < amplitudeSet.size(); j++) {
      if (amplitudeSet[j].real != toInsert.amplitudes[j].real ||
          amplitudeSet[j].imaginary != toInsert.amplitudes[j].imaginary) {
        toInsert.similarity =
            (toInsert.similarity > 0.99999) ? 0.99999 : toInsert.similarity;
        break;
      }
    }

    if (!self->assertionsEqToInsert.contains(instructionIndex)) {
      self->assertionsEqToInsert.insert(
          {instructionIndex, std::vector<InsertEqualityAssertion>{}});
    }
    auto& container = self->assertionsEqToInsert[instructionIndex];
    if (std::ranges::find(container, toInsert) == container.end()) {
      self->assertionsEqToInsert[instructionIndex].push_back(toInsert);
    }
  }
}
} // namespace

Result createDDDiagnostics(DDDiagnostics* self, DDSimulationState* state) {
  self->simulationState = state;

  self->interface.init = dddiagnosticsInit;
  self->interface.getNumQubits = dddiagnosticsGetNumQubits;
  self->interface.getInstructionCount = dddiagnosticsGetInstructionCount;
  self->interface.getInteractions = dddiagnosticsGetInteractions;
  self->interface.getDataDependencies = dddiagnosticsGetDataDependencies;
  self->interface.getZeroControlInstructions =
      dddiagnosticsGetZeroControlInstructions;
  self->interface.potentialErrorCauses = dddiagnosticsPotentialErrorCauses;
  self->interface.suggestAssertionMovements =
      dddiagnosticsSuggestAssertionMovements;
  self->interface.suggestNewAssertions = dddiagnosticsSuggestNewAssertions;

  return self->interface.init(&self->interface);
}

Result destroyDDDiagnostics(DDDiagnostics* self) {
  self->simulationState = nullptr;
  return OK;
}

size_t dddiagnosticsGetNumQubits(Diagnostics* self) {
  const auto* ddd = toDDDiagnostics(self);
  return ddd->simulationState->interface.getNumQubits(
      &ddd->simulationState->interface);
}

size_t dddiagnosticsGetInstructionCount(Diagnostics* self) {
  const auto* ddd = toDDDiagnostics(self);
  return ddd->simulationState->interface.getInstructionCount(
      &ddd->simulationState->interface);
}

Result dddiagnosticsInit(Diagnostics* self) {
  auto* ddd = toDDDiagnostics(self);
  ddd->zeroControls.clear();
  ddd->nonZeroControls.clear();
  ddd->actualQubits.clear();
  return OK;
}

Result dddiagnosticsGetDataDependencies(Diagnostics* self, size_t instruction,
                                        bool includeCallers,
                                        bool* instructions) {
  auto* ddd = toDDDiagnostics(self);
  auto* ddsim = ddd->simulationState;
  const Span<bool> isDependency(
      instructions, ddsim->interface.getInstructionCount(&ddsim->interface));
  std::set<size_t> toVisit{instruction};
  std::set<size_t> visited;

  // Stores all functions whose callers are unknown (because analysis started
  // inside them)
  const std::set<size_t> unknownCallers =
      includeCallers ? getUnknownCallers(ddsim, instruction)
                     : std::set<size_t>{};

  while (!toVisit.empty()) {
    auto current = *toVisit.begin();
    isDependency[current] = true;
    toVisit.erase(toVisit.begin());
    visited.insert(current);

    for (auto dep : ddsim->dataDependencies[current]) {
      const auto depInstruction = dep.first;
      if (ddsim->instructionTypes[depInstruction] == NOP) {
        continue; // We don't want variable declarations as dependencies.
      }
      if (!visited.contains(depInstruction)) {
        toVisit.insert(depInstruction);
      }
      if (ddsim->instructionTypes[depInstruction] == CALL) {
        visitCall(ddsim, depInstruction, dep.second, visited, toVisit);
      }
    }

    if (unknownCallers.contains(current - 1)) {
      for (auto caller : ddsim->functionCallers[current - 1]) {
        if (!visited.contains(caller)) {
          toVisit.insert(caller);
        }
      }
    }
  }

  return OK;
}

Result dddiagnosticsGetInteractions(Diagnostics* self, size_t beforeInstruction,
                                    size_t qubit, bool* qubitsAreInteracting) {
  auto* ddd = toDDDiagnostics(self);
  auto* ddsim = ddd->simulationState;
  std::set<size_t> interactions;
  interactions.insert(qubit);
  bool found = true;

  while (found) {
    found = false;
    for (auto i = beforeInstruction - 1; i < beforeInstruction; i--) {
      if (std::ranges::find(ddsim->functionDefinitions, i) !=
          ddsim->functionDefinitions.end()) {
        break;
      }
      if (ddsim->instructionTypes[i] != SIMULATE &&
          ddsim->instructionTypes[i] != CALL) {
        continue;
      }

      auto targets = getTargetVariables(ddsim, i);
      std::set<size_t> targetQubits;
      for (const auto& target : targets) {
        targetQubits.insert(variableToQubitAt(ddsim, target, i).first);
      }
      if (!std::ranges::none_of(targetQubits, [&interactions](size_t elem) {
            return interactions.contains(elem);
          })) {
        for (const auto& target : targetQubits) {
          if (!interactions.contains(target)) {
            found = true;
          }
          interactions.insert(target);
        }
      }
    }
  }

  const auto qubits = Span<bool>(
      qubitsAreInteracting, ddsim->interface.getNumQubits(&ddsim->interface));
  for (auto interaction : interactions) {
    qubits[interaction] = true;
  }

  return OK;
}

size_t dddiagnosticsPotentialErrorCauses(Diagnostics* self, ErrorCause* output,
                                         size_t count) {
  auto* ddd = toDDDiagnostics(self);
  auto* ddsim = ddd->simulationState;
  auto outputs = Span(output, count);

  const size_t assertion = ddsim->lastFailedAssertion;
  if (assertion == -1ULL) {
    return 0;
  }
  const auto& assertionInstruction = ddsim->assertionInstructions[assertion];
  size_t index = 0;

  if (assertionInstruction->getType() == AssertionType::Entanglement) {
    index +=
        tryFindMissingInteraction(ddd, ddsim, assertion, assertionInstruction,
                                  &outputs[index], count - index);
  }
  index += tryFindZeroControls(ddd, assertion, &outputs[index], count - index);

  return index;
}

size_t tryFindMissingInteraction(DDDiagnostics* diagnostics,
                                 DDSimulationState* state, size_t instruction,
                                 const std::unique_ptr<Assertion>& assertion,
                                 ErrorCause* output, size_t count) {
  if (count == 0) {
    return 0;
  }

  auto targets = assertion->getTargetQubits();
  auto outputs = Span(output, count);
  std::vector<size_t> targetQubits(targets.size());
  size_t index = 0;

  std::ranges::transform(targets, targetQubits.begin(),
                         [&state](const std::string& target) {
                           return variableToQubit(state, target);
                         });

  std::map<size_t, std::set<size_t>> allInteractions;

  for (size_t i = 0; i < targets.size(); i++) {
    allInteractions.insert(
        {targetQubits[i],
         getInteractionsAtRuntime(diagnostics, targetQubits[i])});
  }

  for (size_t i = 0; i < targets.size(); i++) {
    for (size_t j = i + 1; j < targets.size(); j++) {
      if (!allInteractions[targetQubits[i]].contains(targetQubits[j])) {
        outputs[index].type = ErrorCauseType::MissingInteraction;
        outputs[index].instruction = instruction;
        index++;
      }
      if (index == count) {
        return index;
      }
    }
  }

  return index;
}

size_t tryFindZeroControls(DDDiagnostics* diagnostics, size_t instruction,
                           ErrorCause* output, size_t count) {
  if (count == 0) {
    return 0;
  }

  std::vector<uint8_t> dependencies(
      diagnostics->interface.getInstructionCount(&diagnostics->interface));
  diagnostics->interface.getDataDependencies(&diagnostics->interface,
                                             instruction, true,
                                             toBoolArray(dependencies.data()));
  auto outputs = Span(output, count);
  size_t index = 0;

  for (size_t i = 0; i < dependencies.size(); i++) {
    if (dependencies[i] == 0) {
      continue;
    }
    if (!diagnostics->zeroControls.contains(i)) {
      continue;
    }
    if (diagnostics->nonZeroControls.contains(i)) {
      continue;
    }
    const auto& zeroControls = diagnostics->zeroControls[i];
    if (!zeroControls.empty()) {
      outputs[index].type = ErrorCauseType::ControlAlwaysZero;
      outputs[index].instruction = i;
      index++;
      if (index == count) {
        return index;
      }
    }
  }

  return index;
}

Result dddiagnosticsGetZeroControlInstructions(Diagnostics* self,
                                               bool* instructions) {
  auto* ddd = toDDDiagnostics(self);
  const Span<bool> instructionSpan(instructions,
                                   dddiagnosticsGetInstructionCount(self));
  for (size_t i = 0; i < dddiagnosticsGetInstructionCount(self); i++) {
    instructionSpan[i] =
        (!ddd->nonZeroControls.contains(i) && ddd->zeroControls.contains(i));
  }

  return OK;
}

void dddiagnosticsOnStepForward(DDDiagnostics* diagnostics,
                                size_t instruction) {
  auto* ddsim = diagnostics->simulationState;
  const auto targets = getTargetVariables(ddsim, instruction);

  // Add actual qubits to tracker.
  if (ddsim->instructionTypes[instruction] == SIMULATE ||
      ddsim->instructionTypes[instruction] == CALL ||
      ddsim->instructionTypes[instruction] == ASSERTION) {
    std::vector<size_t> targetQubits(targets.size());
    std::ranges::transform(targets, targetQubits.begin(),
                           [&ddsim](const std::string& target) {
                             return variableToQubit(ddsim, target);
                           });
    if (!diagnostics->actualQubits.contains(instruction)) {
      diagnostics->actualQubits[instruction] = std::set<std::vector<size_t>>();
    }
    diagnostics->actualQubits[instruction].insert(targetQubits);
  }

  // Check for zero controls.
  if (ddsim->instructionTypes[instruction] != SIMULATE) {
    return;
  }
  const auto numQubits =
      diagnostics->interface.getNumQubits(&diagnostics->interface);
  if (numQubits > 16) {
    return;
  }
  const auto& op = (*ddsim->iterator);
  const auto& controls = op->getControls();

  std::vector<Complex> amplitudes(2ULL << numQubits);
  Statevector sv{numQubits, 2ULL << numQubits, amplitudes.data()};
  ddsim->interface.getStateVectorFull(&ddsim->interface, &sv);

  for (const auto& control : controls) {
    const auto pos = control.type == qc::Control::Type::Pos;
    const auto qubit = control.qubit;
    if (isAlwaysZero(sv, qubit, !pos)) {
      if (!diagnostics->zeroControls.contains(instruction)) {
        diagnostics->zeroControls[instruction] = std::set<size_t>();
      }
      diagnostics->zeroControls[instruction].insert(qubit);
    } else {
      if (!diagnostics->nonZeroControls.contains(instruction)) {
        diagnostics->nonZeroControls[instruction] = std::set<size_t>();
      }
      diagnostics->nonZeroControls[instruction].insert(qubit);
    }
  }
}

size_t dddiagnosticsSuggestAssertionMovements(Diagnostics* self,
                                              size_t* originalPositions,
                                              size_t* suggestedPositions,
                                              size_t count) {
  DDDiagnostics* diagnostics = toDDDiagnostics(self);
  if (count == 0) {
    return diagnostics->assertionsToMove.size();
  }
  const size_t max = count < diagnostics->assertionsToMove.size()
                         ? count
                         : diagnostics->assertionsToMove.size();
  const Span<size_t> original(originalPositions, count);
  const Span<size_t> suggested(suggestedPositions, count);
  for (size_t i = 0; i < max; i++) {
    original[i] = diagnostics->assertionsToMove[i].first;
    suggested[i] = diagnostics->assertionsToMove[i].second;
  }
  return max;
}

size_t dddiagnosticsSuggestNewAssertions(Diagnostics* self,
                                         size_t* suggestedPositions,
                                         char** suggestedAssertions,
                                         size_t count) {
  auto* ddd = toDDDiagnostics(self);
  if (count == 0) {
    size_t totalNumber = 0;
    for (const auto& entry : ddd->assertionsEntToInsert) {
      totalNumber += entry.second.size();
    }
    for (const auto& entry : ddd->assertionsEqToInsert) {
      totalNumber += entry.second.size();
    }
    return totalNumber;
  }

  size_t index = 0;
  const Span<size_t> positions(suggestedPositions, count);
  const Span<char*> assertions(suggestedAssertions, count);

  for (const auto& entry : ddd->assertionsEntToInsert) {
    for (const auto& assertion : entry.second) {
      positions[index] = assertion.second;
      std::stringstream assertionString;
      assertionString << "assert-ent ";
      for (const auto& qubit : assertion.first) {
        assertionString << qubit + ", ";
      }
      const auto string =
          assertionString.str().substr(0, assertionString.str().size() - 2) +
          ";\n";
      strncpy(assertions[index], string.c_str(), string.length());
      index++;
      if (index == count) {
        return index;
      }
    }
  }

  for (const auto& entry : ddd->assertionsEqToInsert) {
    for (const auto& assertion : entry.second) {
      positions[index] = entry.first;
      std::stringstream assertionString;
      assertionString << "assert-eq ";
      if (assertion.similarity != 1) {
        assertionString << assertion.similarity << ", ";
      }
      for (const auto& qubit : assertion.targets) {
        assertionString << qubit << ", ";
      }
      const auto string =
          assertionString.str().substr(0, assertionString.str().size() - 2);
      std::stringstream finalStringStream;
      finalStringStream << string << " { ";
      const auto& end = assertion.amplitudes.end();
      std::ranges::for_each(assertion.amplitudes,
                            [&finalStringStream, &end](const Complex& c) {
                              if (&c == &*std::prev(end)) {
                                finalStringStream << complexToString(c);
                              } else {
                                finalStringStream << complexToString(c) << ", ";
                              }
                            });
      finalStringStream << " }\n";
      const auto finalString = finalStringStream.str();

      strncpy(assertions[index], finalString.c_str(), finalString.length());
      index++;
      if (index == count) {
        return index;
      }
    }
  }

  return index;
}

void dddiagnosticsOnCodePreprocessing(
    DDDiagnostics* diagnostics, const std::vector<Instruction>& instructions) {
  for (size_t i = 0; i < instructions.size(); i++) {
    const auto& instruction = instructions[i];
    if (instruction.assertion == nullptr) {
      continue;
    }

    size_t lowestSwap = i;

    const auto& assertion = instruction.assertion;
    size_t j = i - 1;
    while (j != -1ULL) {
      if (instructions[j].isFunctionDefinition) {
        break;
      }
      if (instructions[j].code == "RETURN") {
        while (j > 0 && !instructions[j].isFunctionDefinition) {
          j--;
        }
      }
      if (!doesCommute(assertion, instructions[j])) {
        break;
      }
      lowestSwap = j;
      j--;
    }

    if (i != lowestSwap) {
      diagnostics->assertionsToMove.emplace_back(i, lowestSwap);
    }
  }
}

void dddiagnosticsOnFailedAssertion(DDDiagnostics* diagnostics,
                                    size_t instruction) {
  auto* ddsim = diagnostics->simulationState;
  const auto& assertion = ddsim->assertionInstructions[instruction];
  if (assertion->getType() == AssertionType::Entanglement) {
    const auto* entAssertion =
        dynamic_cast<EntanglementAssertion*>(assertion.get());
    suggestBasedOnFailedEntanglementAssertion(diagnostics, instruction,
                                              entAssertion);
  }
  if (assertion->getType() == AssertionType::StatevectorEquality) {
    const auto* eqAssertion =
        dynamic_cast<StatevectorEqualityAssertion*>(assertion.get());
    suggestSplitEqualityAssertion(diagnostics, instruction, eqAssertion);
  }
}

} // namespace mqt::debugger
