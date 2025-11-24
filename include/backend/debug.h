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
 * @file debug.h
 * @brief Provides a C-style interface for the debugging and simulation
 * interface.
 */

#pragma once
// NOLINTBEGIN(modernize-use-using)

#include "common.h"
#include "diagnostics.h"

#ifdef __cplusplus
#include <cstddef>
extern "C" {
#endif

/**
 * @brief A C-style interface for the debugging and simulation interface.
 *
 * This interface provides a way to interact with the simulation state, stepping
 * through the simulation, and inspecting the state of the simulation. \n\n
 *
 * When an executed instruction is a failing assertion, the simulation will stop
 * *before* the assertion and the `didAssertionFail` method will return `true`.
 * Continuing the simulation from there will skip the failing assertion and
 * `didAssertionFail` will return `false` until another assertion fails.
 */
typedef struct SimulationStateStruct SimulationState;

struct SimulationStateStruct {
  /**
   * @brief Initializes the simulation state.
   * @param self The instance to initialize.
   * @return The result of the operation.
   */
  Result (*init)(SimulationState* self);

  /**
   * @brief Loads the given code into the simulation state.
   * @param self The instance to load the code into.
   * @param code The code to load.
   * @return The result of the operation.
   */
  Result (*loadCode)(SimulationState* self, const char* code);

  /**
   * @brief Steps the simulation forward by one instruction.
   * @param self The instance to step forward.
   * @return The result of the operation.
   */
  Result (*stepForward)(SimulationState* self);

  /**
   * @brief Steps the simulation forward by one instruction, skipping over
   * possible custom gate calls.
   * @param self The instance to step forward.
   * @return The result of the operation.
   */
  Result (*stepOverForward)(SimulationState* self);

  /**
   * @brief Steps the simulation forward until the current custom gate call
   * returns.
   * @param self The instance to step forward.
   * @return The result of the operation.
   */
  Result (*stepOutForward)(SimulationState* self);

  /**
   * @brief Steps the simulation backward by one instruction.
   * @param self The instance to step backward.
   * @return The result of the operation.
   */
  Result (*stepBackward)(SimulationState* self);

  /**
   * @brief Steps the simulation backward by one instruction, skipping over
   * possible custom gate calls.
   * @param self The instance to step backward.
   * @return The result of the operation.
   */
  Result (*stepOverBackward)(SimulationState* self);

  /**
   * @brief Steps the simulation backward until the instruction calling the
   * current custom gate is encountered.
   * @param self The instance to step backward.
   * @return The result of the operation.
   */
  Result (*stepOutBackward)(SimulationState* self);

  /**
   * @brief Runs the simulation until it finishes, even if assertions fail.
   * @param self The instance to run.
   * @param failedAssertions A reference to a `size_t` integer to store the
   * number of failed assertions.
   * @return The result of the operation.
   */
  Result (*runAll)(SimulationState* self, size_t* failedAssertions);

  /**
   * @brief Runs the simulation until it finishes or an assertion fails.
   *
   * If an assertion fails, the simulation stops and the `didAssertionFail`
   * method will return `true`. This method is still expected to return `OK`.
   *
   * @param self The instance to run.
   * @return The result of the operation.
   */
  Result (*runSimulation)(SimulationState* self);

  /**
   * @brief Runs the simulation backward until it finishes or an assertion
   * fails.
   * @param self The instance to run.
   * @return The result of the operation.
   */
  Result (*runSimulationBackward)(SimulationState* self);

  /**
   * @brief Resets the simulation to the initial state.
   *
   * This will reset measured variables and state vectors and go back to the
   * start of the code.
   * @param self The instance to reset.
   * @return The result of the operation.
   */
  Result (*resetSimulation)(SimulationState* self);

  /**
   * @brief Pauses the simulation.
   *
   * If the simulation is running in a concurrent thread, the execution will
   * stop as soon as possible, but it is not guaranteed to stop immediately.\n\n
   *
   * If the simulation is not running, then the next call to continue the
   * simulation will stop as soon as possible. `step over` and `step out`
   * methods, in particular, may still execute the next instruction.
   * @param self The instance to pause.
   * @return The result of the operation.
   */
  Result (*pauseSimulation)(SimulationState* self);

  /**
   * @brief Indicates whether the simulation can step forward.
   *
   * The simulation is unable to step forward if it has finished or if the
   * simulation has not been set up yet.
   *
   * @param self The instance to query.
   * @return True if the simulation can step forward, false otherwise.
   */
  bool (*canStepForward)(SimulationState* self);

  /**
   * @brief Indicates whether the simulation can step backward.
   *
   * The simulation is unable to step backward if it is at the beginning or if
   * the simulation has not been set up yet.
   *
   * @param self The instance to query.
   * @return True if the simulation can step backward, false otherwise.
   */
  bool (*canStepBackward)(SimulationState* self);

  /**
   * @brief Toggles the value of a classical bit variable.
   *
   * @param self The instance to query.
   * @param variableName The name of the classical bit that should be toggled.
   * @return The result of the operation.
   */
  Result (*changeClassicalVariable)(SimulationState* self,
                                    const char* variableName);
  /**
   * @brief Changes the amplitude of a computational basis state.
   *
   * @param self The instance to query.
   * @param basisState The bitstring identifying the basis state to update.
   * @param value The desired complex amplitude.
   * @return The result of the operation.
   */
  Result (*changeAmplitudeVariable)(SimulationState* self,
                                    const char* basisState,
                                    const Complex* value);

  /**
   * @brief Indicates whether the execution has finished.
   *
   * The execution is considered finished if it has reached the end of the code.
   * @param self The instance to query.
   * @return True if the execution has finished, false otherwise.
   */
  bool (*isFinished)(SimulationState* self);

  /**
   * @brief Indicates whether an assertion has failed in the previous step.
   *
   * If execution is continued after a failed assertion, then this flag will
   * be set to false again.
   * @param self The instance to query.
   * @return True if an assertion has failed, false otherwise.
   */
  bool (*didAssertionFail)(SimulationState* self);

  /**
   * @brief Indicates whether a breakpoint was hit in the previous step.
   *
   * If execution is continued after a breakpoint, then this flag will
   * be set to false again.
   * @param self The instance to query.
   * @return True if a breakpoint was hit, false otherwise.
   */
  bool (*wasBreakpointHit)(SimulationState* self);

  /**
   * @brief Gets the current instruction index.
   * @param self The instance to query.
   * @return The current instruction index.
   */
  size_t (*getCurrentInstruction)(SimulationState* self);

  /**
   * @brief Gets the number of instructions in the code.
   * @param self The instance to query.
   * @return The total number of instructions.
   */
  size_t (*getInstructionCount)(SimulationState* self);

  /**
   * @brief Gets the position of the given instruction in the code.
   *
   * Start and end positions are inclusive and white-spaces are ignored.
   * @param self The instance to query.
   * @param instruction The instruction index.
   * @param start A reference to a `size_t` integer to store the start position.
   * @param end A reference to a `size_t` integer to store the end position.
   * @return The result of the operation.
   */
  Result (*getInstructionPosition)(SimulationState* self, size_t instruction,
                                   size_t* start, size_t* end);

  /**
   * @brief Gets the number of qubits used by the program.
   * @param self The instance to query.
   * @return The number of qubits.
   */
  size_t (*getNumQubits)(SimulationState* self);

  /**
   * @brief Gets the complex amplitude of a state in the full state vector.
   *
   * The amplitude is selected by an integer index that corresponds to the
   * binary representation of the state.
   *
   * @param self The instance to query.
   * @param index The index of the state.
   * @param output A reference to a `Complex` instance to store the amplitude.
   * @return The result of the operation.
   */
  Result (*getAmplitudeIndex)(SimulationState* self, size_t index,
                              Complex* output);

  /**
   * @brief Gets the complex amplitude of a state in the full state vector.
   *
   * The amplitude is selected by a bitstring representing the state.
   *
   * @param self The instance to query.
   * @param bitstring The index of the state as a bitstring.
   * @param output A reference to a `Complex` instance to store the amplitude.
   * @return The result of the operation.
   */
  Result (*getAmplitudeBitstring)(SimulationState* self, const char* bitstring,
                                  Complex* output);

  /**
   * @brief Gets a classical variable by name.
   *
   * For registers, the name should be the register name followed by the index
   * in square brackets.
   *
   * @param self The instance to query.
   * @param name The name of the variable.
   * @param output A reference to a `Variable` instance to store the variable.
   * @return The result of the operation.
   */
  Result (*getClassicalVariable)(SimulationState* self, const char* name,
                                 Variable* output);

  /**
   * @brief Gets the number of classical variables in the simulation.
   *
   * For registers, each index is counted as a separate variable.
   * @param self The instance to query.
   * @return The number of classical variables.
   */
  size_t (*getNumClassicalVariables)(SimulationState* self);

  /**
   * @brief Gets the name of a classical variable by its index.
   *
   * For registers, each index is counted as a separate variable and can be
   * accessed separately. This method will return the name of the specific
   * index of the register.
   * @param self The instance to query.
   * @param variableIndex The index of the variable.
   * @param output A buffer to store the name of the variable.
   * @return The result of the operation.
   */
  Result (*getClassicalVariableName)(SimulationState* self,
                                     size_t variableIndex, char* output);

  /**
   * @brief Gets the name of a quantum variable by its index.
   *
   * For registers, each index is counted as a separate variable and can be
   * accessed separately. This method will return the name of the specific
   * index of the register.
   * @param self The instance to query.
   * @param variableIndex The index of the variable.
   * @param output A buffer to store the name of the variable.
   * @return The result of the operation.
   */
  Result (*getQuantumVariableName)(SimulationState* self, size_t variableIndex,
                                   char* output);

  /**
   * @brief Gets the full state vector of the simulation at the current time.
   *
   * The state vector is expected to be initialized with the correct number of
   * qubits and allocated space for the amplitudes before calling this method.
   * @param self The instance to query.
   * @param output A reference to a `Statevector` instance to store the state
   * vector.
   * @return The result of the operation.
   */
  Result (*getStateVectorFull)(SimulationState* self, Statevector* output);

  /**
   * @brief Gets a sub-state of the state vector of the simulation at the
   * current time.
   *
   * The state vector is expected to be initialized with the correct number of
   * qubits and allocated space for the amplitudes before calling this method.
   * \n\n
   *
   * This method also supports the re-ordering of qubits, but does not allow
   * qubits to be repeated.
   *
   * @param self The instance to query.
   * @param subStateSize The number of qubits in the sub-state.
   * @param qubits An array of qubit indices to include in the sub-state.
   * @param output The state vector to store the sub-state.
   * @return The result of the operation.
   */
  Result (*getStateVectorSub)(SimulationState* self, size_t subStateSize,
                              const size_t* qubits, Statevector* output);

  /**
   * @brief Sets a breakpoint at the desired position in the code.
   *
   * The position is given as a 0-indexed character position in the full code
   * string. The instruction at which the breakpoint was set is returned in the
   * `targetInstruction` parameter.
   * @param self The instance to set the breakpoint in.
   * @param desiredPosition The desired position in the code as a 0-indexed
   * character index.
   * @param targetInstruction A reference to a `size_t` integer to store the
   * target instruction.
   * @return The result of the operation.
   */
  Result (*setBreakpoint)(SimulationState* self, size_t desiredPosition,
                          size_t* targetInstruction);

  /**
   * @brief Clears all breakpoints set in the simulation.
   * @param self The instance to clear the breakpoints in.
   * @return The result of the operation.
   */
  Result (*clearBreakpoints)(SimulationState* self);

  /**
   * @brief Gets the current stack depth of the simulation.
   *
   * Each custom gate call corresponds to one stack entry.\n\n
   *
   * @param self The instance to query.
   * @param depth A reference to a `size_t` integer to store the stack depth.
   * @return The result of the operation.
   */
  Result (*getStackDepth)(SimulationState* self, size_t* depth);

  /**
   * @brief Gets the current stack trace of the simulation.
   *
   * The stack trace is represented as a list of instruction indices. Each
   * instruction index represents a single return address for the corresponding
   * stack entry. \n\n
   *
   * This method expects a continuous memory block of integers with size equal
   * to the maximum depth. Each element represents a stack entry and will be set
   * to the instruction index of the return address.
   *
   * @param self The instance to query.
   * @param maxDepth The maximum depth of the stack trace.
   * @param output A buffer to store the stack trace.
   * @return The result of the operation.
   */
  Result (*getStackTrace)(SimulationState* self, size_t maxDepth,
                          size_t* output);

  /**
   * @brief Gets the diagnostics interface instance employed by this debugger.
   * @param self The instance to query.
   * @return The diagnostics interface instance.
   */
  Diagnostics* (*getDiagnostics)(SimulationState* self);

  /**
   * @brief Compiles the given code into a quantum circuit without assertions.
   * @param self The SimulationState instance from which the original assertion
   * code should be taken.
   * @param buffer The buffer that should be filled with the compiled code, or
   * NULL to compute the required buffer size.
   * @param settings The settings to use for the compilation.
   * @return The size of the compiled code.
   */
  size_t (*compile)(SimulationState* self, char* buffer,
                    CompilationSettings settings);
};

#ifdef __cplusplus
}
#endif

// NOLINTEND(modernize-use-using)
