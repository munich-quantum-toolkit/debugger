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
 * @file diagnostics.h
 * @brief Provides a C-style interface for diagnostic capabilities of the
 * debugger.
 */

#pragma once

// NOLINTBEGIN(modernize-use-using, performance-enum-size)

#include "common.h"

#ifdef __cplusplus
#include <cstddef>
extern "C" {
#endif

/**
 * @brief Represents the type of an error cause.
 *
 * These error causes are returned by the `potentialErrorCauses` function.
 */
typedef enum {
  /**
   * @brief Indicates that the error cause is unknown.
   */
  Unknown,
  /**
   * @brief Indicates that an entanglement error may be caused by a missing
   * interaction.
   */
  MissingInteraction,
  /**
   * @brief Indicates that an error may be related to a controlled gate with a
   * control that is always zero.
   */
  ControlAlwaysZero
} ErrorCauseType;

/**
 * @brief Represents an error cause.
 */
typedef struct ErrorCauseStruct ErrorCause;

struct ErrorCauseStruct {
  /**
   * @brief The type of the error cause.
   */
  ErrorCauseType type;
  /**
   * @brief The instruction where the error may originate from or where the
   * error can be detected.
   */
  size_t instruction;
};

/**
 * @brief An interface representing the diagnostic capabilities of a debugger.
 */
typedef struct DiagnosticsStruct Diagnostics;

struct DiagnosticsStruct {
  /**
   * @brief Initializes the diagnostics interface.
   * @param self The instance to initialize.
   * @return The result of the operation.
   */
  Result (*init)(Diagnostics* self);

  /**
   * @brief Get the number of qubits in the system.
   * @param self The diagnostics instance to query.
   * @return The number of qubits.
   */
  size_t (*getNumQubits)(Diagnostics* self);

  /**
   * @brief Get the number of instructions in the code.
   * @param self The diagnostics instance to query.
   * @return The number of instructions.
   */
  size_t (*getInstructionCount)(Diagnostics* self);

  /**
   * @brief Extract all data dependencies for a given instruction.
   *
   * This method expects a continuous memory block of booleans with size equal
   * to the number of instructions. Each element represents an instruction and
   * will be set to true if the instruction is a data dependency.\n\n
   *
   * If the instruction is inside a custom gate definition, the data
   * dependencies will by default not go outside of the custom gate, unless a
   * new call instruction is found. By setting `includeCallers` to true, all
   * possible callers of the custom gate will also be included and further
   * dependencies outside the custom gate will be taken from there.\n\n
   *
   * The line itself will also be counted as a dependency. Gate and register
   * declarations will not.\n\n
   *
   * This method can be performed without running the program, as it is a static
   * analysis method.\n\n
   *
   * @param self The diagnostics instance to query.
   * @param instruction The instruction to extract the data dependencies for.
   * @param includeCallers True, if the data dependencies should include all
   * possible callers of the containing custom gate.
   * @param instructions An array of booleans that will be set to true for each
   * instruction that is a data dependency.
   * @return The result of the operation.
   */
  Result (*getDataDependencies)(Diagnostics* self, size_t instruction,
                                bool includeCallers, bool* instructions);

  /**
   * @brief Extract all qubits that interact with a given qubit up to a specific
   * instruction.
   *
   * This method expects a continuous memory block of booleans with size equal
   * to the number of qubits. Each element represents a qubit and will be set to
   * true if the qubit interacts with the given qubit.\n\n
   *
   * If the target instruction is inside a custom gate definition, the
   * interactions will only be searched inside the custom gate, unless a new
   * call instruction is found.\n\n
   *
   * The qubit itself will also be counted as an interaction.\n\n
   *
   * This method can be performed without running the program, as it is a static
   * analysis method.\n\n
   *
   * @param self The diagnostics instance to query.
   * @param beforeInstruction The instruction to extract the interactions up to
   * (excluding).
   * @param qubit The qubit to extract the interactions for.
   * @param qubitsAreInteracting An array of booleans that will be set to true
   * for each qubit that interacts with the given qubit.
   * @return The result of the operation.
   */
  Result (*getInteractions)(Diagnostics* self, size_t beforeInstruction,
                            size_t qubit, bool* qubitsAreInteracting);

  /**
   * @brief Extract all controlled gates that have been marked as only having
   * controls with value zero.
   *
   * This method expects a continuous memory block of booleans with size equal
   * to the number of instructions. Each element represents an instruction and
   * will be set to true if the instruction is a controlled gate with only zero
   * controls.\n\n
   *
   * This method can only be performed at runtime, as it is a dynamic analysis
   * method.\n\n
   *
   * @param self The diagnostics instance to query.
   * @param instructions An array of booleans that will be set to true for each
   * instruction that is a controlled gate with only zero controls.
   * @return The result of the operation.
   */
  Result (*getZeroControlInstructions)(Diagnostics* self, bool* instructions);

  /**
   * @brief Extract a list of potential error causes encountered during
   * execution.
   *
   * Up to `count` error causes will be returned.
   *
   * This method should be run after the program has been executed and reached
   * an assertion error.
   * @param self The diagnostics instance to query.
   * @param output An array of error causes to be filled. It is expected to have
   * space for at least `count` elements.
   * @param count The maximum number of error causes to return.
   * @return The number of error causes found.
   */
  size_t (*potentialErrorCauses)(Diagnostics* self, ErrorCause* output,
                                 size_t count);

  /**
   * @brief Suggest movements of assertions to better positions.\n\n
   *
   * Calling this function with a `count` of 0 will return the number of
   * assertions that can be suggested.
   *
   * @param self The diagnostics instance to query.
   * @param originalPositions An array of assertion positions to be filled.
   * Contains the original positions of the assertions that should be moved.
   * @param suggestedPositions An array of assertion positions to be filled.
   * Contains the suggested positions of the assertions that should be moved.
   * @param count The maximum number of assertions to suggest movements for.
   * @return The number of suggested movements.
   */
  size_t (*suggestAssertionMovements)(Diagnostics* self,
                                      size_t* originalPositions,
                                      size_t* suggestedPositions, size_t count);

  /**
   * @brief Suggest new assertions to be added to the code.
   *
   * These assertions are added by first observing assertions that failed during
   * previous iterations. Therefore, the simulation must be run at least once
   * before calling this function.\n\n
   *
   * Calling this function with a `count` of 0 will return the number of
   * assertions that can be suggested.
   *
   * @param self The diagnostics instance to query.
   * @param suggestedPositions An array of assertion positions to be filled.
   * @param suggestedAssertions An array of assertion instruction strings to be
   * filled. Each string expects a size of up to 256 characters.
   * @param count The maximum number of assertions to suggest.
   * @return The number of suggested assertions.
   */
  size_t (*suggestNewAssertions)(Diagnostics* self, size_t* suggestedPositions,
                                 char** suggestedAssertions, size_t count);
};

#ifdef __cplusplus
}
#endif

// NOLINTEND(modernize-use-using, performance-enum-size)
