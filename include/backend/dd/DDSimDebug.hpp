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
 * @file DDSimDebug.hpp
 * @brief Contains the implementation of the `SimulationState` interface for the
 * DD simulator and related data structures and tools.
 */
#pragma once

#include "DDSimDiagnostics.hpp"
#include "backend/debug.h"
#include "backend/diagnostics.h"
#include "common.h"
#include "common/parsing/AssertionParsing.hpp"
#include "dd/Package.hpp"
#include "ir/QuantumComputation.hpp"
#include "ir/operations/Operation.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace mqt::debugger {

/**
 * @brief Represents the different types of an instruction.
 */
enum InstructionType : uint8_t {
  /**
   * @brief Nothing happens when this instruction is encountered, as the
   * simulation backend already handled it at "compile"-time.
   */
  NOP,
  /**
   * @brief The instruction is a quantum operation that has to be simulated on
   * the simulation backend.
   */
  SIMULATE,
  /**
   * @brief The instruction is an assertion that has to be checked.
   */
  ASSERTION,
  /**
   * @brief The instruction is a custom gate call.
   */
  CALL,
  /**
   * @brief The instruction returns from a custom gate call.
   */
  RETURN
};

/**
 * @brief Represents a qubit register in the code.
 */
struct QubitRegisterDefinition {
  /**
   * @brief The name of the register.
   */
  std::string name;
  /**
   * @brief The index of the register in the set of quantum variables.
   */
  size_t index;
  /**
   * @brief The size of the register.
   */
  size_t size;
};

/**
 * @brief Represents a classical register in the code.
 */
struct ClassicalRegisterDefinition {
  /**
   * @brief The name of the register.
   */
  std::string name;
  /**
   * @brief The index of the register in the set of classical variables.
   */
  size_t index;
  /**
   * @brief The size of the register.
   */
  size_t size;
};

/**
 * @brief The DD-simulator implementation of the `SimulationState` interface.
 */
struct DDSimulationState {
  /**
   * @brief The `SimulationState` interface.
   */
  SimulationState interface;
  /**
   * @brief The current instruction during execution.
   */
  size_t currentInstruction;
  /**
   * @brief The code being executed.
   */
  std::string code;
  /**
   * @brief The code being executed, after preprocessing.
   */
  std::string processedCode;
  /**
   * @brief Indicates whether the debugger is ready to start simulation.
   */
  bool ready;

  /**
   * @brief The quantum computation object used for simulation.
   */
  std::unique_ptr<qc::QuantumComputation> qc;

  /**
   * @brief The DD package used for simulation.
   */
  std::unique_ptr<dd::Package> dd;
  /**
   * @brief The iterator pointing to the current instruction in the simulation.
   */
  std::vector<std::unique_ptr<qc::Operation>>::iterator iterator;
  /**
   * @brief The DD vector representing the current simulation state.
   */
  dd::VectorDD simulationState;
  /**
   * @brief A vector containing the `InstructionType` of each instruction.
   */
  std::vector<InstructionType> instructionTypes;
  /**
   * @brief A vector containing the start positions in the code of each
   * instruction.
   */
  std::vector<size_t> instructionStarts;
  /**
   * @brief A vector containing the end positions in the code of each
   * instruction.
   */
  std::vector<size_t> instructionEnds;
  /**
   * @brief A vector containing the instruction indices of all function
   * definitions.
   */
  std::set<size_t> functionDefinitions;
  /**
   * @brief A map containing the instruction indices of all assertion, mapped to
   * their `Assertion` objects.
   */
  std::map<size_t, std::unique_ptr<Assertion>> assertionInstructions;
  /**
   * @brief Maps each instruction to its successor instruction.
   */
  std::map<size_t, size_t> successorInstructions;
  /**
   * @brief A vector containing all qubit registers.
   *
   * This vector is already populated before the simulation starts and the
   * respective declaration is found.
   */
  std::vector<QubitRegisterDefinition> qubitRegisters;
  /**
   * @brief A vector containing all classical registers.
   *
   * This vector is already populated before the simulation starts and the
   * respective declaration is found.
   */
  std::vector<ClassicalRegisterDefinition> classicalRegisters;
  /**
   * @brief Maps the names of all classical variables to their values.
   */
  std::map<std::string, Variable> variables;
  /**
   * @brief A vector containing the names of all classical variables.
   */
  std::vector<std::unique_ptr<std::string>> variableNames;
  /**
   * @brief The current stack of previous instructions. Stepping backward pops
   * this stack.
   *
   * The stack will be cleared when a measurement is encountered.
   */
  std::vector<size_t> previousInstructionStack;
  /**
   * @brief The current stack of return instructions. Reaching a `RETURN`
   * instruction pops this stack.
   */
  std::vector<size_t> callReturnStack;
  /**
   * @brief Maps each custom gate call instruction to the substitutions for this
   * call.
   *
   * The substitution maps the names of the parameters of the custom gate to the
   * names of the variables used to call the custom gate.
   */
  std::map<size_t, std::map<std::string, std::string>> callSubstitutions;
  /**
   * @brief Saves elements removed from the `callReturnStack` so that they can
   * be reused when stepping back.
   */
  std::vector<std::pair<size_t, size_t>> restoreCallReturnStack;
  /**
   * @brief Maps each instruction index to a vector of its immediate data
   * dependencies.
   *
   * For each variable used by this instruction, this vector contains a
   * reference to the last instruction that used it. It also contains the index
   * of the variable in that dependency's argument list, so that it can be
   * identified exactly.
   */
  std::map<size_t, std::vector<std::pair<size_t, size_t>>> dataDependencies;
  /**
   * @brief Maps each custom gate definition to a set of all instructions that
   * call it.
   */
  std::map<size_t, std::set<size_t>> functionCallers;
  /**
   * @brief A set of all breakpoints set by the user.
   */
  std::set<size_t> breakpoints;
  /**
   * @brief A vector containing the names of all target qubits for each
   * instruction.
   */
  std::vector<std::vector<std::string>> targetQubits;

  /**
   * @brief Indicates whether the simulation should be paused.
   */
  bool paused;

  /**
   * @brief Stores the last instruction that failed an assertion.
   */
  size_t lastFailedAssertion;
  /**
   * @brief Stores the last instruction that hit a breakpoint.
   */
  size_t lastMetBreakpoint;

  /**
   * @brief The diagnostics instance used for analysis.
   */
  DDDiagnostics diagnostics;

  /**
   * @brief Object representations of all parsed instructions.
   */
  std::vector<Instruction> instructionObjects;
};

/**
 * @brief Initializes the simulation state.
 * @param self The instance to initialize.
 * @return The result of the operation.
 */
Result ddsimInit(SimulationState* self);

/**
 * @brief Loads the given code into the simulation state.
 * @param self The instance to load the code into.
 * @param code The code to load.
 * @return The result of the operation.
 */
Result ddsimLoadCode(SimulationState* self, const char* code);
/**
 * @brief Steps the simulation forward by one instruction.
 * @param self The instance to step forward.
 * @return The result of the operation.
 */
Result ddsimStepForward(SimulationState* self);
/**
 * @brief Steps the simulation backward by one instruction.
 * @param self The instance to step backward.
 * @return The result of the operation.
 */
Result ddsimStepBackward(SimulationState* self);
/**
 * @brief Steps the simulation forward by one instruction, skipping over
 * possible custom gate calls.
 * @param self The instance to step forward.
 * @return The result of the operation.
 */
Result ddsimStepOverForward(SimulationState* self);
/**
 * @brief Steps the simulation backward by one instruction, skipping over
 * possible custom gate calls.
 * @param self The instance to step backward.
 * @return The result of the operation.
 */
Result ddsimStepOverBackward(SimulationState* self);
/**
 * @brief Steps the simulation forward until the current custom gate call
 * returns.
 * @param self The instance to step forward.
 * @return The result of the operation.
 */
Result ddsimStepOutForward(SimulationState* self);
/**
 * @brief Steps the simulation backward until the instruction calling the
 * current custom gate is encountered.
 * @param self The instance to step backward.
 * @return The result of the operation.
 */
Result ddsimStepOutBackward(SimulationState* self);
/**
 * @brief Runs the simulation until it finishes, even if assertions fail.
 * @param self The instance to run.
 * @param failedAssertions A reference to a `size_t` integer to store the number
 * of failed assertions.
 * @return The result of the operation.
 */
Result ddsimRunAll(SimulationState* self, size_t* failedAssertions);
/**
 * @brief Runs the simulation until it finishes or an assertion fails.
 *
 * If an assertion fails, the simulation stops and the `didAssertionFail` method
 * will return `true`. This method is still expected to return `OK`.
 *
 * @param self The instance to run.
 * @return The result of the operation.
 */
Result ddsimRunSimulation(SimulationState* self);
/**
 * @brief Runs the simulation backward until it finishes or an assertion fails.
 * @param self The instance to run.
 * @return The result of the operation.
 */
Result ddsimRunSimulationBackward(SimulationState* self);
/**
 * @brief Resets the simulation to the initial state.
 *
 * This will reset measured variables and state vectors and go back to the
 * start of the code.
 * @param self The instance to reset.
 * @return The result of the operation.
 */
Result ddsimResetSimulation(SimulationState* self);
/**
 * @brief Pauses the simulation.
 *
 * If the simulation is running in a concurrent thread, the execution will stop
 * as soon as possible, but it is not guaranteed to stop immediately.\n\n
 *
 * If the simulation is not running, then the next call to continue the
 * simulation will stop as soon as possible. `step over` and `step out` methods,
 * in particular, may still execute the next instruction.
 * @param self The instance to pause.
 * @return The result of the operation.
 */
Result ddsimPauseSimulation(SimulationState* self);
/**
 * @brief Checks whether the simulation can step forward.
 *
 * The simulation is unable to step forward if it has finished or if the
 * simulation has not been set up yet.
 *
 * @param self The instance to query.
 * @return True if the simulation can step forward, false otherwise.
 */
bool ddsimCanStepForward(SimulationState* self);
/**
 * @brief Checks whether the simulation can step backward.
 *
 * The simulation is unable to step backward if it is at the beginning or if the
 * simulation has not been set up yet.
 *
 * @param self The instance to query.
 * @return True if the simulation can step backward, false otherwise.
 */
bool ddsimCanStepBackward(SimulationState* self);
/**
 * @brief Checks whether the simulation has finished.
 *
 * The simulation is considered finished if it has reached the end of the code
 * @param self The instance to query.
 * @return True if the simulation has finished, false otherwise.
 */
bool ddsimIsFinished(SimulationState* self);
/**
 * @brief Checks whether an assertion has failed in the previous step.
 *
 * If execution is continued after a failed assertion, then this flag will
 * be set to false again.
 * @param self The instance to query.
 * @return True if an assertion has failed, false otherwise.
 */
bool ddsimDidAssertionFail(SimulationState* self);
/**
 * @brief Checks whether a breakpoint was hit in the previous step.
 *
 * If execution is continued after a breakpoint, then this flag will
 * be set to false again.
 * @param self The instance to query.
 * @return True if a breakpoint was hit, false otherwise.
 */
bool ddsimWasBreakpointHit(SimulationState* self);

/**
 * @brief Gets the current instruction index.
 * @param self The instance to query.
 * @return The current instruction index.
 */
size_t ddsimGetCurrentInstruction(SimulationState* self);
/**
 * @brief Gets the number of instructions in the code.
 * @param self The instance to query.
 * @return The total number of instructions.
 */
size_t ddsimGetInstructionCount(SimulationState* self);
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
Result ddsimGetInstructionPosition(SimulationState* self, size_t instruction,
                                   size_t* start, size_t* end);

/**
 * @brief Gets the number of qubits used by the program.
 * @param self The instance to query.
 * @return The number of qubits.
 */
size_t ddsimGetNumQubits(SimulationState* self);
/**
 * @brief Gets the complex amplitude of a state in the full state vector.
 *
 * The amplitude is selected by an integer index that corresponds to the binary
 * representation of the state.
 *
 * @param self The instance to query.
 * @param index The index of the state.
 * @param output A reference to a `Complex` instance to store the amplitude.
 * @return The result of the operation.
 */
Result ddsimGetAmplitudeIndex(SimulationState* self, size_t index,
                              Complex* output);
/**
 * @brief Gets the complex amplitude of a state in the full state vector.
 *
 * The amplitude is selected by a bitstring representing the state.
 *
 * @param self The instance to query.
 * @param bitstring The index of the qubit as a bitstring.
 * @param output A reference to a `Complex` instance to store the amplitude.
 * @return The result of the operation.
 */
Result ddsimGetAmplitudeBitstring(SimulationState* self, const char* bitstring,
                                  Complex* output);

/**
 * @brief Gets a classical variable by name.
 *
 * For registers, the name should be the register name followed by the index in
 * square brackets.
 *
 * @param self The instance to query.
 * @param name The name of the variable.
 * @param output A reference to a `Variable` instance to store the variable.
 * @return The result of the operation.
 */
Result ddsimGetClassicalVariable(SimulationState* self, const char* name,
                                 Variable* output);

/**
 * @brief change the value of the current classical bit variable to its opposite value.
 * 
 * @param self The instance to query.
 * @return The result of the operation.
 */
Result ddsimChangeClassicalVariable(SimulationState* self);
/**
 * @brief Gets the number of classical variables in the simulation.
 *
 * For registers, each index is counted as a separate variable.
 * @param self The instance to query.
 * @return The number of classical variables.
 */
size_t ddsimGetNumClassicalVariables(SimulationState* self);

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
Result ddsimGetClassicalVariableName(SimulationState* self,
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
Result ddsimGetQuantumVariableName(SimulationState* self, size_t variableIndex,
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
Result ddsimGetStateVectorFull(SimulationState* self, Statevector* output);
/**
 * @brief Gets a sub-state of the state vector of the simulation at the current
 * time.
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
Result ddsimGetStateVectorSub(SimulationState* self, size_t subStateSize,
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
Result ddsimSetBreakpoint(SimulationState* self, size_t desiredPosition,
                          size_t* targetInstruction);
/**
 * @brief Clears all breakpoints set in the simulation.
 * @param self The instance to clear the breakpoints in.
 * @return The result of the operation.
 */
Result ddsimClearBreakpoints(SimulationState* self);
/**
 * @brief Gets the current stack depth of the simulation.
 *
 * Each custom gate call corresponds to one stack entry.\n\n
 *
 * @param self The instance to query.
 * @param depth A reference to a `size_t` integer to store the stack depth.
 * @return The result of the operation.
 */
Result ddsimGetStackDepth(SimulationState* self, size_t* depth);
/**
 * @brief Gets the current stack trace of the simulation.
 *
 * The stack trace is represented as a list of instruction indices. Each
 * instruction index represents a single return address for the corresponding
 * stack entry. \n\n
 *
 * This method expects a continuous memory block of integers with size equal to
 * the maximum depth. Each element represents a stack entry and will be set to
 * the instruction index of the return address.
 *
 * @param self The instance to query.
 * @param maxDepth The maximum depth of the stack trace.
 * @param output A buffer to store the stack trace.
 * @return The result of the operation.
 */
Result ddsimGetStackTrace(SimulationState* self, size_t maxDepth,
                          size_t* output);

/**
 * @brief Gets the diagnostics interface instance employed by this debugger.
 * @param self The instance to query.
 * @return The diagnostics interface instance.
 */
Diagnostics* ddsimGetDiagnostics(SimulationState* self);

/**
 * @brief Compiles the given code into a quantum circuit without assertions.
 * @param self The SimulationState instance from which the original assertion
 * code should be taken.
 * @param buffer The buffer that should be filled with the compiled code, or
 * NULL to compute the required buffer size.
 * @param settings The settings to use for the compilation.
 * @return The size of the compiled code.
 */
size_t ddsimCompile(SimulationState* self, char* buffer,
                    CompilationSettings settings);

/**
 * @brief Creates a new `DDSimulationState` instance.
 *
 * This function expects an allocated memory block for the `DDSimulationState`
 * instance.
 * @param self The instance to create.
 * @return The result of the operation.
 */
Result createDDSimulationState(DDSimulationState* self);
/**
 * @brief Destroys a `DDSimulationState` instance.
 * @param self The instance to destroy.
 * @return The result of the operation.
 */
Result destroyDDSimulationState(DDSimulationState* self);

/**
 * @brief Preprocess the code to be executed.
 * @param code The code to preprocess.
 * @param ddsim The simulation state to preprocess the code for.
 * @return The preprocessed code.
 */
std::string preprocessAssertionCode(const char* code, DDSimulationState* ddsim);

/**
 * @brief Checks an assertion against the current state of the simulation.
 * @param ddsim The simulation state to check the assertion against.
 * @param assertion The assertion to check.
 * @return True if the assertion is satisfied, false otherwise.
 */
bool checkAssertion(DDSimulationState* ddsim,
                    std::unique_ptr<Assertion>& assertion);

/**
 * @brief Gets the name of a classical bit by its index.
 * @param ddsim The simulation state to query.
 * @param index The index of the classical bit.
 * @return The name of the classical bit.
 */
std::string getClassicalBitName(DDSimulationState* ddsim, size_t index);

/**
 * @brief Gets the name of a qubit variable by its index.
 * @param ddsim The simulation state to query.
 * @param index The index of the qubit variable.
 * @return The name of the qubit variable.
 */
std::string getQuantumBitName(DDSimulationState* ddsim, size_t index);

/**
 * @brief Gets the qubit index from a variable name.
 *
 * If the variable is in the global scope, the index is based on the index of
 * the qubit in the state vector. If the variable is in a function scope, it
 * uses the current substitution to check what actual quantum variable is used
 * and computes the index based on that.
 * @param ddsim The simulation state to query.
 * @param variable The name of the variable.
 * @return The index of the qubit.
 */
size_t variableToQubit(DDSimulationState* ddsim, const std::string& variable);

/**
 * @brief Gets the qubit index from a variable name.
 *
 * If the variable is in the global scope, the index is based on the index of
 * the qubit in the state vector. If the variable is in a function scope, the
 * index is based on the index of the qubit in the function's parameter list.
 * @param ddsim The simulation state to query.
 * @param variable The name of the variable.
 * @return The index of the qubit.
 */
std::pair<size_t, size_t> variableToQubitAt(DDSimulationState* ddsim,
                                            const std::string& variable,
                                            size_t instruction);

/**
 * @brief Checks whether the set of target qubits can be used to compute a
 * sub-state vector.
 *
 * For that, none of the target qubits may be entangled with any non-target
 * qubits.
 * @param full The full state vector.
 * @param targetQubits The target qubits.
 * @return True if the sub-state vector can be computed, false otherwise.
 */
bool isSubStateVectorLegal(const Statevector& full,
                           std::vector<size_t>& targetQubits);

/**
 * @brief Gets the target variables of an instruction.
 *
 * If the instruction targets are indexed registers, they are taken directly.
 * If the instruction targets are full registers, they are replaced by each
 * individual qubit in the register.
 * @param ddsim The simulation state to query.
 * @param instruction The instruction index to get the target variables for.
 * @return The target variables.
 */
std::vector<std::string> getTargetVariables(DDSimulationState* ddsim,
                                            size_t instruction);

/**
 * @brief Compiles the given code into a quantum circuit without assertions
 * using statistical slices.
 * @param ddsim The DDSimulationState instance from which the original assertion
 * code should be taken.
 * @param buffer The buffer that should be filled with the compiled code, or
 * NULL to compute the required buffer size.
 * @param settings The settings to use for the compilation.
 * @return The size of the compiled code.
 */
size_t compileStatisticalSlice(DDSimulationState* ddsim, char* buffer,
                               CompilationSettings settings);

} // namespace mqt::debugger
