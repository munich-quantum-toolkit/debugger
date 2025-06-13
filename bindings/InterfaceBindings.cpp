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
 * @file InterfaceBindings.cpp
 * @brief Implementation of the Python bindings for the general debugging and
 * diagnostics interfaces.
 */

#include "backend/debug.h"
#include "backend/diagnostics.h"
#include "common.h"
#include "pybind11/cast.h"
#include "pybind11/pybind11.h"
#include "pybind11/stl.h" // NOLINT(misc-include-cleaner)

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <pybind11/detail/common.h>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace py = pybind11;
using namespace pybind11::literals;

namespace mqt::debugger {

namespace {
/**
 * @brief Checks whether the given result is OK, and throws a runtime_error
 * otherwise.
 * @param result The result to check.
 */
void checkOrThrow(Result result) {
  if (result != OK) {
    throw std::runtime_error("An error occurred while executing the operation");
  }
}
} // namespace

/**
 * @brief A representation of statevectors in C++ style, using std::vector
 * instead of a raw pointer.
 *
 * This is used to make the statevector more easily accessible from Python.
 */
struct StatevectorCPP {
  size_t numQubits = 0;
  size_t numStates = 0;
  std::vector<Complex> amplitudes;
};

void bindFramework(py::module& m) {
  // Bind the VariableType enum
  py::enum_<VariableType>(m, "VariableType",
                          "The type of a classical variable.")
      .value("VarBool", VarBool, "A boolean variable.")
      .value("VarInt", VarInt, "An integer variable.")
      .value("VarFloat", VarFloat, "A floating-point variable.")
      .export_values();

  // Bind the VariableValue union
  py::class_<VariableValue>(m, "VariableValue")
      .def(py::init<>())
      .def_readwrite("bool_value", &VariableValue::boolValue,
                     "The value of a boolean variable.")
      .def_readwrite("int_value", &VariableValue::intValue,
                     "The value of an integer variable.")
      .def_readwrite("float_value", &VariableValue::floatValue,
                     "The value of a floating-point variable.")
      .doc() = R"(Represents the value of a classical variable.

Only one of these fields has a valid value at a time, based on the variable's `VariableType`.)";

  // Bind the Variable struct
  py::class_<Variable>(m, "Variable")
      .def(py::init<>(), "Creates a new `Variable` instance.")
      .def_readwrite("name", &Variable::name, "The name of the variable.")
      .def_readwrite("type", &Variable::type, "The type of the variable.")
      .def_readwrite("value", &Variable::value, "The value of the variable.")
      .doc() = "Represents a classical variable.";

  // Bind the Complex struct
  py::class_<Complex>(m, "Complex")
      .def(py::init<>(), R"(Initializes a new complex number.

Args:
   real (float, optional): The real part of the complex number. Defaults to 0.0.
   imaginary (float, optional): The imaginary part of the complex number. Defaults to 0.0.)")
      .def(py::init<double, double>(), R"(Initializes a new complex number.

Args:
   real (float, optional): The real part of the complex number. Defaults to 0.0.
   imaginary (float, optional): The imaginary part of the complex number. Defaults to 0.0.)")
      .def_readwrite("real", &Complex::real,
                     "The real part of the complex number.")
      .def_readwrite("imaginary", &Complex::imaginary,
                     "The imaginary part of the complex number.")
      .def(
          "__str__",
          [](const Complex& self) {
            return std::to_string(self.real) + " + " +
                   std::to_string(self.imaginary) + "i";
          },
          R"(Returns a string representation of the complex number.
Returns:
   str: The string representation of the complex number.
)")
      .def(
          "__repr__",
          [](const Complex& self) {
            return "Complex(" + std::to_string(self.real) + ", " +
                   std::to_string(self.imaginary) + ")";
          },
          R"(Returns a string representation of the complex number.

Returns:
   str: The string representation of the complex number.)")
      .doc() = "Represents a complex number.";

  // Bind the Statevector struct
  py::class_<StatevectorCPP>(m, "Statevector")
      .def(py::init<>(), "Creates a new `Statevector` instance.")
      .def_readwrite("num_qubits", &StatevectorCPP::numQubits,
                     "The number of qubits in the state vector.")
      .def_readwrite("num_states", &StatevectorCPP::numStates,
                     R"(The number of states in the state vector.

This is always equal to 2^`num_qubits`.)")
      .def_readwrite("amplitudes", &StatevectorCPP::amplitudes,
                     R"(The amplitudes of the state vector.

Contains one element for each of the `num_states` states in the state vector.)")
      .doc() = "Represents a state vector.";

  py::class_<CompilationSettings>(m, "CompilationSettings")
      .def(py::init<uint8_t, size_t>(), py::arg("opt"),
           py::arg("slice_index") = 0,
           R"(Initializes a new set of compilation settings.

Args:
   opt (int): The optimization level that should be used.
   slice_index (int, optional): The index of the slice that should be compiled (defaults to 0).)")
      .def_readwrite(
          "opt", &CompilationSettings ::opt,
          "The optimization level that should be used. Exact meaning depends "
          "on "
          "the implementation, but typically 0 means no optimization.")
      .def_readwrite("slice_index", &CompilationSettings::sliceIndex,
                     "The index of the slice that should be compiled.")
      .doc() =
      "The settings that should be used to compile an assertion program.";

  py::class_<SimulationState>(m, "SimulationState")
      .def(py::init<>(), "Creates a new `SimulationState` instance.")
      .def(
          "init", [](SimulationState* self) { checkOrThrow(self->init(self)); },
          "Initializes the simulation state.")
      .def(
          "load_code",
          [](SimulationState* self, const char* code) {
            checkOrThrow(self->loadCode(self, code));
          },
          R"(Loads the given code into the simulation state.

Args:
    code (str): The code to load.)")
      .def(
          "step_forward",
          [](SimulationState* self) { checkOrThrow(self->stepForward(self)); },
          "Steps the simulation forward by one instruction.")
      .def(
          "step_over_forward",
          [](SimulationState* self) {
            checkOrThrow(self->stepOverForward(self));
          },
          "Steps the simulation forward by one instruction, skipping over "
          "possible custom gate calls.")
      .def(
          "step_out_forward",
          [](SimulationState* self) {
            checkOrThrow(self->stepOutForward(self));
          },
          "Steps the simulation forward until the current custom gate call "
          "returns.")
      .def(
          "step_backward",
          [](SimulationState* self) { checkOrThrow(self->stepBackward(self)); },
          "Steps the simulation backward by one instruction.")
      .def(
          "step_over_backward",
          [](SimulationState* self) {
            checkOrThrow(self->stepOverBackward(self));
          },
          "Steps the simulation backward by one instruction, skipping over "
          "possible custom gate calls.")
      .def(
          "step_out_backward",
          [](SimulationState* self) {
            checkOrThrow(self->stepOutBackward(self));
          },
          "Steps the simulation backward until the instruction calling the "
          "current custom gate is encountered.")
      .def(
          "run_all",
          [](SimulationState* self) {
            size_t errors = 0;
            checkOrThrow(self->runAll(self, &errors));
            return errors;
          },
          R"(Runs the simulation until it finishes, even if assertions fail.

Returns:
int: The number of assertions that failed during execution.)")
      .def(
          "run_simulation",
          [](SimulationState* self) {
            checkOrThrow(self->runSimulation(self));
          },
          R"(Runs the simulation until it finishes or an assertion fails.

If an assertion fails, the simulation stops and the `did_assertion_fail`
method will return `True`.)")
      .def(
          "run_simulation_backward",
          [](SimulationState* self) {
            checkOrThrow(self->runSimulationBackward(self));
          },
          "Runs the simulation backward until it finishes or an assertion "
          "fails.")
      .def(
          "reset_simulation",
          [](SimulationState* self) {
            checkOrThrow(self->resetSimulation(self));
          },
          R"(Resets the simulation to the initial state.

This will reset measured variables and state vectors and go back to the
start of the code.)")
      .def(
          "pause_simulation",
          [](SimulationState* self) {
            checkOrThrow(self->pauseSimulation(self));
          },
          R"(Pauses the simulation.

If the simulation is running in a concurrent thread, the execution will
stop as soon as possible, but it is not guaranteed to stop immediately.

If the simulation is not running, then the next call to continue the
simulation will stop as soon as possible. `step over` and `step out`
methods, in particular, may still execute the next instruction.)")
      .def(
          "can_step_forward",
          [](SimulationState* self) { return self->canStepForward(self); },
          R"(Indicates whether the simulation can step forward.

The simulation is unable to step forward if it has finished or if the
simulation has not been set up yet.

Returns:
bool: True, if the simulation can step forward.)")
      .def(
          "can_step_backward",
          [](SimulationState* self) { return self->canStepBackward(self); },
          R"(Indicates whether the simulation can step backward.

The simulation is unable to step backward if it is at the beginning or if
the simulation has not been set up yet.

Returns:
bool: True, if the simulation can step backward.)")
      .def(
          "is_finished",
          [](SimulationState* self) { return self->isFinished(self); },
          R"(Indicates whether the execution has finished.

The execution is considered finished if it has reached the end of the code.

Returns:
bool: True, if the execution has finished.)")
      .def(
          "did_assertion_fail",
          [](SimulationState* self) { return self->didAssertionFail(self); },
          R"(Indicates whether an assertion has failed in the previous step.

If execution is continued after a failed assertion, then this flag will
be set to false again.

Returns:
bool: True, if an assertion failed during the last step.)")
      .def(
          "was_breakpoint_hit",
          [](SimulationState* self) { return self->wasBreakpointHit(self); },
          R"(Indicates whether a breakpoint was hit in the previous step.

If execution is continued after a breakpoint, then this flag will
be set to false again.

Returns:
bool: True, if a breakpoint was hit during the last step.)")
      .def(
          "get_current_instruction",
          [](SimulationState* self) {
            return self->getCurrentInstruction(self);
          },
          R"(Gets the current instruction index.

Returns:
int: The index of the current instruction.)")
      .def(
          "get_instruction_count",
          [](SimulationState* self) { return self->getInstructionCount(self); },
          R"(Gets the number of instructions in the code.

Returns:
    int: The number of instructions in the code.)")
      .def(
          "get_instruction_position",
          [](SimulationState* self, size_t instruction) {
            size_t start = 0;
            size_t end = 0;
            checkOrThrow(
                self->getInstructionPosition(self, instruction, &start, &end));
            return std::make_pair(start, end);
          },
          R"(Gets the position of the given instruction in the code.

Start and end positions are inclusive and white-spaces are ignored.

Args:
    instruction (int): The instruction to find.

Returns:
    tuple[int, int]: The start and end positions of the instruction.)")
      .def(
          "get_num_qubits",
          [](SimulationState* self) { return self->getNumQubits(self); },
          R"(Gets the number of qubits used by the program.

Returns:
    int: The number of qubits used by the program.)")
      .def(
          "get_amplitude_index",
          [](SimulationState* self, size_t qubit) {
            Complex output;
            checkOrThrow(self->getAmplitudeIndex(self, qubit, &output));
            return output;
          },
          R"(Gets the complex amplitude of a state in the full state vector.

The amplitude is selected by an integer index that corresponds to the
binary representation of the state.

Args:
    index (int): The index of the state in the full state vector.

Returns:
    Complex: The complex amplitude of the state.)")
      .def(
          "get_amplitude_bitstring",
          [](SimulationState* self, const char* bitstring) {
            Complex output;
            checkOrThrow(self->getAmplitudeBitstring(self, bitstring, &output));
            return output;
          },
          R"(Gets the complex amplitude of a state in the full state vector.

The amplitude is selected by a bitstring representing the state.

Args:
    bitstring (str): The index of the state as a bitstring.

Returns:
    Complex: The complex amplitude of the state.)")
      .def(
          "get_classical_variable",
          [](SimulationState* self, const char* name) {
            Variable output;
            checkOrThrow(self->getClassicalVariable(self, name, &output));
            return output;
          },
          R"(Gets a classical variable by name.

For registers, the name should be the register name followed by the index
in square brackets.

Args:
    name (str): The name of the variable.

Returns:
    Variable: The fetched variable.)")
      .def(
          "get_num_classical_variables",
          [](SimulationState* self) {
            return self->getNumClassicalVariables(self);
          },
          R"(Gets the number of classical variables in the simulation.

For registers, each index is counted as a separate variable.

Returns:
    int: The number of classical variables in the simulation.)")
      .def(
          "get_classical_variable_name",
          [](SimulationState* self, size_t variableIndex) {
            std::string output(255, '\0');
            checkOrThrow(self->getClassicalVariableName(self, variableIndex,
                                                        output.data()));
            const std::size_t pos = output.find_first_of('\0');
            if (pos != std::string::npos) {
              output = output.substr(0, pos);
            }
            return output;
          },
          R"(Gets the name of a classical variable by its index.

For registers, each index is counted as a separate variable and can be
accessed separately. This method will return the name of the specific
index of the register.

Args:
    index (int): The index of the variable.

Returns:
    str: The name of the variable.)")
      .def(
          "get_quantum_variable_name",
          [](SimulationState* self, size_t variableIndex) {
            std::string output(255, '\0');
            checkOrThrow(self->getQuantumVariableName(self, variableIndex,
                                                      output.data()));
            const std::size_t pos = output.find_first_of('\0');
            if (pos != std::string::npos) {
              output = output.substr(0, pos);
            }
            return output;
          },
          R"(Gets the name of a quantum variable by its index.

For registers, each index is counted as a separate variable and can be
accessed separately. This method will return the name of the specific
index of the register.

Args:
    index (int): The index of the variable.

Returns:
    str: The name of the variable.)")
      .def(
          "get_state_vector_full",
          [](SimulationState* self) {
            const size_t numQubits = self->getNumQubits(self);
            const std::vector<Complex> amplitudes(1 << numQubits);
            StatevectorCPP result{numQubits, 1ULL << numQubits, amplitudes};
            Statevector output{numQubits, result.numStates,
                               result.amplitudes.data()};
            checkOrThrow(self->getStateVectorFull(self, &output));
            return result;
          },
          R"(Gets the full state vector of the simulation at the current time.

The state vector is expected to be initialized with the correct number of
qubits and allocated space for the amplitudes before calling this method.

Returns:
    Statevector: The full state vector of the current simulation state.)")
      .def(
          "get_state_vector_sub",
          [](SimulationState* self, std::vector<size_t> qubits) {
            const size_t numQubits = qubits.size();
            const std::vector<Complex> amplitudes(1 << numQubits);
            StatevectorCPP result{numQubits, 1ULL << numQubits, amplitudes};
            Statevector output{numQubits, result.numStates,
                               result.amplitudes.data()};
            checkOrThrow(self->getStateVectorSub(self, numQubits, qubits.data(),
                                                 &output));
            return result;
          },
          R"(Gets a sub-state of the state vector of the simulation at the current time.

The state vector is expected to be initialized with the correct number of
qubits and allocated space for the amplitudes before calling this method.

This method also supports the re-ordering of qubits, but does not allow
qubits to be repeated.

Args:
    qubits (list[int]): The qubits to include in the sub-state.

Returns:
    Statevector: The sub-state vector of the current simulation state.)")
      .def(
          "set_breakpoint",
          [](SimulationState* self, size_t desiredPosition) {
            size_t actualPosition = 0;
            checkOrThrow(
                self->setBreakpoint(self, desiredPosition, &actualPosition));
            return actualPosition;
          },
          R"(Sets a breakpoint at the desired position in the code.

The position is given as a 0-indexed character position in the full code
string.

Args:
    desired_position (int): The position in the code to set the breakpoint.

Returns:
    int: The index of the instruction where the breakpoint was set.)")
      .def(
          "clear_breakpoints",
          [](SimulationState* self) {
            checkOrThrow(self->clearBreakpoints(self));
          },
          "Clears all breakpoints set in the simulation.")
      .def(
          "get_stack_depth",
          [](SimulationState* self) {
            size_t depth = 0;
            checkOrThrow(self->getStackDepth(self, &depth));
            return depth;
          },
          R"(Gets the current stack depth of the simulation.

Each custom gate call corresponds to one stack entry.

Returns:
    int: The current stack depth of the simulation.)")
      .def(
          "get_stack_trace",
          [](SimulationState* self, size_t maxDepth) {
            size_t trueSize = 0;
            checkOrThrow(self->getStackDepth(self, &trueSize));
            const size_t stackSize = std::min(maxDepth, trueSize);
            std::vector<size_t> stackTrace(stackSize);
            checkOrThrow(
                self->getStackTrace(self, maxDepth, stackTrace.data()));
            return stackTrace;
          },
          R"(Gets the current stack trace of the simulation.

The stack trace is represented as a list of instruction indices. Each
instruction index represents a single return address for the corresponding
stack entry.

Args:
    max_depth (int): The maximum depth of the stack trace.

Returns:
    list[int]: The stack trace of the simulation.)")
      .def(
          "get_diagnostics",
          [](SimulationState* self) { return self->getDiagnostics(self); },
          py::return_value_policy::reference_internal,
          R"(Gets the diagnostics instance employed by this debugger.

Returns:
    Diagnostics: The diagnostics instance employed by this debugger.)")
      .def(
          "compile",
          [](SimulationState* self, CompilationSettings& settings) {
            const auto size = self->compile(self, nullptr, settings);
            if (size == 0) {
              return std::string();
            }
            std::vector<char> buffer(size);
            self->compile(self, buffer.data(), settings);
            std::string result(buffer.data(), size - 1);
            return result;
          },
          R"(Compiles the given code into a quantum circuit without assertions.

Args:
    settings (CompilationSettings): The settings to use for the compilation.

Returns:
  str: The compiled code.)")
      .doc() = R"(Represents the state of a quantum simulation for debugging.

"This is the main class of the `mqt-debugger` library, allowing developers to step through the code and inspect the state of the simulation.)";
}

void bindDiagnostics(py::module& m) {
  // Bind the ErrorCauseType enum
  py::enum_<ErrorCauseType>(m, "ErrorCauseType",
                            "The type of a potential error cause.")
      .value("Unknown", Unknown, "An unknown error cause.")
      .value("MissingInteraction", MissingInteraction,
             "Indicates that an entanglement error may be caused by a missing "
             "interaction.")
      .value("ControlAlwaysZero", ControlAlwaysZero,
             "Indicates that an error may be related to a controlled gate with "
             "a control that is always zero.")
      .export_values();

  // Bind the ErrorCause struct
  py::class_<ErrorCause>(m, "ErrorCause")
      .def(py::init<>())
      .def_readwrite("instruction", &ErrorCause ::instruction,
                     "The instruction where the error may originate from or "
                     "where the error can be detected.")
      .def_readwrite("type", &ErrorCause ::type,
                     "The type of the potential error cause.")
      .doc() = "Represents a potential cause of an assertion error.";

  py::class_<Diagnostics>(m, "Diagnostics")
      .def(py::init<>(), "Creates a new `Diagnostics` instance.")
      .def(
          "init", [](Diagnostics* self) { checkOrThrow(self->init(self)); },
          "Initializes the diagnostics instance.")
      .def(
          "get_num_qubits",
          [](Diagnostics* self) { return self->getNumQubits(self); },
          R"(Get the number of qubits in the system.

Returns:
   int: The number of qubits in the system.)")
      .def(
          "get_instruction_count",
          [](Diagnostics* self) { return self->getInstructionCount(self); },
          R"(Get the number of instructions in the code.

Returns:
   int: The number of instructions in the code.)")
      .def(
          "get_data_dependencies",
          [](Diagnostics* self, size_t instruction, bool includeCallers) {
            std::vector<uint8_t> instructions(self->getInstructionCount(self));
            // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
            checkOrThrow(self->getDataDependencies(
                self, instruction, includeCallers,
                reinterpret_cast<bool*>(instructions.data())));
            // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
            std::vector<size_t> result;
            for (size_t i = 0; i < instructions.size(); i++) {
              if (instructions[i] != 0) {
                result.push_back(i);
              }
            }
            return result;
          },
          py::arg("instruction"), py::arg("include_callers") = false,
          R"(Extract all data dependencies for a given instruction.

If the instruction is inside a custom gate definition, the data
dependencies will by default not go outside of the custom gate, unless a
new call instruction is found. By setting `include_callers` to `True`, all
possible callers of the custom gate will also be included and further
dependencies outside the custom gate will be taken from there.

The line itself will also be counted as a dependency. Gate and register
declarations will not.

This method can be performed without running the program, as it is a static
analysis method.

Args:
   instruction (int): The instruction to extract the data dependencies for.
   include_callers (bool, optional): True, if the data dependencies should include all possible callers of the containing custom gate. Defaults to False.

Returns:
   list[int]: A list of instruction indices that are data dependencies of the given instruction.)")
      .def(
          "get_interactions",
          [](Diagnostics* self, size_t beforeInstruction, size_t qubit) {
            std::vector<uint8_t> qubits(self->getNumQubits(self));
            // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
            checkOrThrow(
                self->getInteractions(self, beforeInstruction, qubit,
                                      reinterpret_cast<bool*>(qubits.data())));
            // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
            std::vector<size_t> result;
            for (size_t i = 0; i < qubits.size(); i++) {
              if (qubits[i] != 0) {
                result.push_back(i);
              }
            }
            return result;
          },
          R"(Extract all qubits that interact with a given qubit up to a specific instruction.

If the target instruction is inside a custom gate definition, the
interactions will only be searched inside the custom gate, unless a new
call instruction is found.

The qubit itself will also be counted as an interaction.

This method can be performed without running the program, as it is a static
analysis method.

Args:
   before_instruction (int): The instruction to extract the interactions up to (excluding).
   qubit (int): The qubit to extract the interactions for.

Returns:
   list[int]: A list of qubit indices that interact with the given qubit up to the target instruction.)")
      .def(
          "get_zero_control_instructions",
          [](Diagnostics* self) {
            std::vector<uint8_t> instructions(self->getInstructionCount(self));
            // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
            checkOrThrow(self->getZeroControlInstructions(
                self, reinterpret_cast<bool*>(instructions.data())));
            // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
            std::vector<size_t> result;
            for (size_t i = 0; i < instructions.size(); i++) {
              if (instructions[i] != 0) {
                result.push_back(i);
              }
            }
            return result;
          },
          R"(Extract all controlled gates that have been marked as only having controls with value zero.

This method expects a continuous memory block of booleans with size equal
to the number of instructions. Each element represents an instruction and
will be set to true if the instruction is a controlled gate with only zero
controls.

This method can only be performed at runtime, as it is a dynamic analysis
method.

Returns:
   list[int]: The indices of instructions that are controlled gates with only zero controls.)")
      .def(
          "potential_error_causes",
          [](Diagnostics* self) {
            size_t nextSize = 10;
            while (true) {
              std::vector<ErrorCause> output(nextSize);
              const auto actualSize =
                  self->potentialErrorCauses(self, output.data(), nextSize);
              if (actualSize <= nextSize) {
                output.resize(actualSize);
                return output;
              }
              nextSize = nextSize * 2;
            }
          },
          R"(Extract a list of potential error causes encountered during execution.

This method should be run after the program has been executed and reached
an assertion error.

Returns:
   list[ErrorCause]: A list of potential error causes encountered during execution.)")
      .def(
          "suggest_assertion_movements",
          [](Diagnostics* self) {
            const size_t count =
                self->suggestAssertionMovements(self, nullptr, nullptr, 0);
            std::vector<size_t> originalPositions(count);
            std::vector<size_t> suggestedPositions(count);
            self->suggestAssertionMovements(self, originalPositions.data(),
                                            suggestedPositions.data(), count);
            std::vector<std::pair<size_t, size_t>> result(count);
            std::transform(originalPositions.begin(), originalPositions.end(),
                           suggestedPositions.begin(), result.begin(),
                           [](const size_t& a, const size_t& b) {
                             return std::make_pair(a, b);
                           });
            return result;
          },
          R"(Suggest movements of assertions to better positions.

Each entry of the resulting list consists of the original position of the assertion, followed by its new
suggested position.

Returns:
  list[tuple[int, int]]: A list of moved assertions.
)")
      .def(
          "suggest_new_assertions",
          [](Diagnostics* self) {
            const size_t stringSize = 2 << 17;
            const size_t count =
                self->suggestNewAssertions(self, nullptr, nullptr, 0);
            std::vector<size_t> positions(count);
            std::vector<char*> buffers(count);
            for (auto& b : buffers) {
              char* buffer = reinterpret_cast<char*>(
                  malloc(sizeof(char) * stringSize)); // NOLINT
              for (size_t i = 0; i < stringSize; i++) {
                buffer[i] = '\0';
              }
              b = buffer;
            }

            self->suggestNewAssertions(self, positions.data(), buffers.data(),
                                       count);
            std::vector<std::string> assertions;
            for (auto* b : buffers) {
              assertions.emplace_back(b);
              free(b); // NOLINT
            }
            std::vector<std::pair<size_t, std::string>> result(count);

            std::transform(positions.begin(), positions.end(),
                           assertions.begin(), result.begin(),
                           [](const size_t& a, const std::string& b) {
                             return std::make_pair(a, b);
                           });
            return result;
          },
          R"(Suggest new assertions to be added to the program.

Each entry of the resulting list consists of the suggested position for the new assertion, followed by its
string representation.

Returns:
  list[tuple[int, str]]: A list of new assertions.
)")
      .doc() = "Provides diagnostics capabilities such as different analysis "
               "methods for the debugger.";
}

} // namespace mqt::debugger
