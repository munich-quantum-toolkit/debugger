# Copyright (c) 2024 - 2026 Chair for Design Automation, TUM
# Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

import enum
from collections.abc import Sequence
from typing import overload

class Result(enum.Enum):
    """Represents the result of an operation."""

    OK = 0
    """Indicates that the operation was successful."""
    ERROR = 1
    """Indicates that an error occurred."""

class ErrorCauseType(enum.Enum):
    """The type of a potential error cause."""

    Unknown = 0
    """An unknown error cause."""

    MissingInteraction = 1
    """
    Indicates that an entanglement error may be caused by a missing interaction.
    """

    ControlAlwaysZero = 2
    """
    Indicates that an error may be related to a controlled gate with a control that is always zero.
    """

class ErrorCause:
    """Represents a potential cause of an assertion error."""

    def __init__(self) -> None: ...
    @property
    def instruction(self) -> int:
        """The instruction where the error may originate from or where the error can be detected."""

    @instruction.setter
    def instruction(self, arg: int, /) -> None: ...
    @property
    def type_(self) -> ErrorCauseType:
        """The type of the potential error cause."""

    @type_.setter
    def type_(self, arg: ErrorCauseType, /) -> None: ...

class Diagnostics:
    """Provides diagnostics capabilities such as different analysis methods for the debugger."""

    def __init__(self) -> None:
        """Creates a new `Diagnostics` instance."""

    def init(self) -> None:
        """Initializes the diagnostics instance."""

    def get_num_qubits(self) -> int:
        """Get the number of qubits in the system.

        Returns:
            The number of qubits in the system.
        """

    def get_instruction_count(self) -> int:
        """Get the number of instructions in the code.

        Returns:
            The number of instructions in the code.
        """

    def get_data_dependencies(self, instruction: int, include_callers: bool = False) -> list[int]:
        """Extract all data dependencies for a given instruction.

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
            instruction: The instruction to extract the data dependencies for.
            include_callers: True, if the data dependencies should include all possible callers of the containing custom gate. Defaults to False.

        Returns:
            A list of instruction indices that are data dependencies of the given instruction.
        """

    def get_interactions(self, before_instruction: int, qubit: int) -> list[int]:
        """Extract all qubits that interact with a given qubit up to a specific instruction.

        If the target instruction is inside a custom gate definition, the
        interactions will only be searched inside the custom gate, unless a new
        call instruction is found.

        The qubit itself will also be counted as an interaction.

        This method can be performed without running the program, as it is a static
        analysis method.

        Args:
            before_instruction: The instruction to extract the interactions up to (excluding).
            qubit: The qubit to extract the interactions for.

        Returns:
            A list of qubit indices that interact with the given qubit up to the target instruction.
        """

    def get_zero_control_instructions(self) -> list[int]:
        """Extract all controlled gates that have been marked as only having controls with value zero.

        This method expects a continuous memory block of booleans with size equal
        to the number of instructions. Each element represents an instruction and
        will be set to true if the instruction is a controlled gate with only zero
        controls.

        This method can only be performed at runtime, as it is a dynamic analysis
        method.

        Returns:
            The indices of instructions that are controlled gates with only zero controls.
        """

    def potential_error_causes(self) -> list[ErrorCause]:
        """Extract a list of potential error causes encountered during execution.

        This method should be run after the program has been executed and reached
        an assertion error.

        Returns:
           A list of potential error causes encountered during execution.
        """

    def suggest_assertion_movements(self) -> list[tuple[int, int]]:
        """Suggest movements of assertions to better positions.

        Each entry of the resulting list consists of the original position of the assertion, followed by its new
        suggested position.

        Returns:
            A list of moved assertions.
        """

    def suggest_new_assertions(self) -> list[tuple[int, str]]:
        """Suggest new assertions to be added to the program.

        Each entry of the resulting list consists of the suggested position for the new assertion, followed by its
        string representation.

        Returns:
            A list of new assertions.
        """

class VariableType(enum.Enum):
    """The type of a classical variable."""

    VarBool = 0
    """A boolean variable."""

    VarInt = 1
    """An integer variable."""

    VarFloat = 2
    """A floating-point variable."""

class VariableValue:
    """Represents the value of a classical variable.

    Only one of these fields has a valid value at a time, based on the variable's `VariableType`.
    """

    def __init__(self) -> None: ...
    @property
    def bool_value(self) -> bool:
        """The value of a boolean variable."""

    @bool_value.setter
    def bool_value(self, arg: bool, /) -> None: ...
    @property
    def int_value(self) -> int:
        """The value of an integer variable."""

    @int_value.setter
    def int_value(self, arg: int, /) -> None: ...
    @property
    def float_value(self) -> float:
        """The value of a floating-point variable."""

    @float_value.setter
    def float_value(self, arg: float, /) -> None: ...

class Variable:
    """Represents a classical variable."""

    def __init__(self) -> None:
        """Creates a new `Variable` instance."""

    @property
    def name(self) -> str:
        """The name of the variable."""

    @name.setter
    def name(self, arg: str, /) -> None: ...
    @property
    def type_(self) -> VariableType:
        """The type of the variable."""

    @type_.setter
    def type_(self, arg: VariableType, /) -> None: ...
    @property
    def value(self) -> VariableValue:
        """The value of the variable."""

    @value.setter
    def value(self, arg: VariableValue, /) -> None: ...

class Complex:
    """Represents a complex number."""

    @overload
    def __init__(self) -> None:
        """Initializes a new complex number."""

    @overload
    def __init__(self, real: float = 0.0, imaginary: float = 0.0) -> None:
        """Initializes a new complex number.

        Args:
            real: The real part of the complex number. Defaults to 0.0.
            imaginary: The imaginary part of the complex number. Defaults to 0.0.
        """

    @property
    def real(self) -> float:
        """The real part of the complex number."""

    @real.setter
    def real(self, arg: float, /) -> None: ...
    @property
    def imaginary(self) -> float:
        """The imaginary part of the complex number."""

    @imaginary.setter
    def imaginary(self, arg: float, /) -> None: ...

class Statevector:
    """Represents a state vector."""

    def __init__(self) -> None:
        """Creates a new `Statevector` instance."""

    @property
    def num_qubits(self) -> int:
        """The number of qubits in the state vector."""

    @num_qubits.setter
    def num_qubits(self, arg: int, /) -> None: ...
    @property
    def num_states(self) -> int:
        """The number of states in the state vector.

        This is always equal to 2^`num_qubits`.
        """

    @num_states.setter
    def num_states(self, arg: int, /) -> None: ...
    @property
    def amplitudes(self) -> list[Complex]:
        """The amplitudes of the state vector.

        Contains one element for each of the `num_states` states in the state vector.
        """

    @amplitudes.setter
    def amplitudes(self, arg: Sequence[Complex], /) -> None: ...

class CompilationSettings:
    """The settings that should be used to compile an assertion program."""

    def __init__(self, opt: int, slice_index: int = 0) -> None:
        """Initializes a new set of compilation settings.

        Args:
            opt: The optimization level that should be used.
            slice_index: The index of the slice that should be compiled (defaults to 0).
        """

    @property
    def opt(self) -> int:
        """The optimization level that should be used. Exact meaning depends on the implementation, but typically 0 means no optimization."""

    @opt.setter
    def opt(self, arg: int, /) -> None: ...
    @property
    def slice_index(self) -> int:
        """The index of the slice that should be compiled."""

    @slice_index.setter
    def slice_index(self, arg: int, /) -> None: ...

class SimulationState:
    """Represents the state of a quantum simulation for debugging.

    This is the main class of the `mqt-debugger` library, allowing developers to step through the code and inspect the state of the simulation.
    """

    def __init__(self) -> None:
        """Creates a new `SimulationState` instance."""

    def init(self) -> None:
        """Initializes the simulation state."""

    def load_code(self, code: str) -> None:
        """Loads the given code into the simulation state.

        Args:
            code: The code to load.
        """

    def load_code_with_result(self, code: str) -> LoadResult:
        """Loads the given code and returns details about any errors.

        Args:
            code (str): The code to load.

        Returns:
            LoadResult: The result of the load operation.
        """

    def step_forward(self) -> None:
        """Steps the simulation forward by one instruction."""

    def step_over_forward(self) -> None:
        """Steps the simulation forward by one instruction, skipping over possible custom gate calls."""

    def step_out_forward(self) -> None:
        """Steps the simulation forward until the current custom gate call returns."""

    def step_backward(self) -> None:
        """Steps the simulation backward by one instruction."""

    def step_over_backward(self) -> None:
        """Steps the simulation backward by one instruction, skipping over possible custom gate calls."""

    def step_out_backward(self) -> None:
        """Steps the simulation backward until the instruction calling the current custom gate is encountered."""

    def run_all(self) -> int:
        """Runs the simulation until it finishes, even if assertions fail.

        Returns:
            The number of assertions that failed during execution.
        """

    def run_simulation(self) -> None:
        """Runs the simulation until it finishes or an assertion fails.

        If an assertion fails, the simulation stops and the `did_assertion_fail`
        method will return `True`.
        """

    def run_simulation_backward(self) -> None:
        """Runs the simulation backward until it finishes or an assertion fails."""

    def reset_simulation(self) -> None:
        """Resets the simulation to the initial state.

        This will reset measured variables and state vectors and go back to the
        start of the code.
        """

    def pause_simulation(self) -> None:
        """Pauses the simulation.

        If the simulation is running in a concurrent thread, the execution will
        stop as soon as possible, but it is not guaranteed to stop immediately.

        If the simulation is not running, then the next call to continue the
        simulation will stop as soon as possible. `step over` and `step out`
        methods, in particular, may still execute the next instruction.
        """

    def can_step_forward(self) -> bool:
        """Indicates whether the simulation can step forward.

        The simulation is unable to step forward if it has finished or if the
        simulation has not been set up yet.

        Returns:
            True, if the simulation can step forward.
        """

    def can_step_backward(self) -> bool:
        """Indicates whether the simulation can step backward.

        The simulation is unable to step backward if it is at the beginning or if
        the simulation has not been set up yet.

        Returns:
            True, if the simulation can step backward.
        """

    def change_classical_variable_value(self, variable_name: str, new_value: object) -> None:
        """Updates the value of a classical variable.

        Args:
            variable_name: The name of the classical variable to update.
            new_value: The desired value.
        """

    def change_amplitude_value(self, basis_state: str, value: Complex) -> None:
        """Updates the amplitude of a given computational basis state.

        The basis state is provided as a bitstring whose length matches the
        current number of qubits. Implementations are expected to renormalize the
        remaining amplitudes so that the state vector stays normalized and to
        reject invalid bitstrings or amplitudes that violate normalization.

        Args:
            basis_state: The bitstring identifying the basis state to update.
            value: The desired complex amplitude.
        """

    def is_finished(self) -> bool:
        """Indicates whether the execution has finished.

        The execution is considered finished if it has reached the end of the code.

        Returns:
            True, if the execution has finished.
        """

    def did_assertion_fail(self) -> bool:
        """Indicates whether an assertion has failed in the previous step.

        If execution is continued after a failed assertion, then this flag will
        be set to false again.

        Returns:
            True, if an assertion failed during the last step.
        """

    def was_breakpoint_hit(self) -> bool:
        """Indicates whether a breakpoint was hit in the previous step.

        If execution is continued after a breakpoint, then this flag will
        be set to false again.

        Returns:
            True, if a breakpoint was hit during the last step.
        """

    def get_current_instruction(self) -> int:
        """Gets the current instruction index.

        Returns:
            The index of the current instruction.
        """

    def get_instruction_count(self) -> int:
        """Gets the number of instructions in the code.

        Returns:
            The number of instructions in the code.
        """

    def get_instruction_position(self, instruction: int) -> tuple[int, int]:
        """Gets the position of the given instruction in the code.

        Start and end positions are inclusive and white-spaces are ignored.

        Args:
            instruction: The instruction to find.

        Returns:
            The start and end positions of the instruction.
        """

    def get_num_qubits(self) -> int:
        """Gets the number of qubits used by the program.

        Returns:
            The number of qubits used by the program.
        """

    def get_amplitude_index(self, index: int) -> Complex:
        """Gets the complex amplitude of a state in the full state vector.

        The amplitude is selected by an integer index that corresponds to the
        binary representation of the state.

        Args:
            index: The index of the state in the full state vector.

        Returns:
            The complex amplitude of the state.
        """

    def get_amplitude_bitstring(self, bitstring: str) -> Complex:
        """Gets the complex amplitude of a state in the full state vector.

        The amplitude is selected by a bitstring representing the state.

        Args:
            bitstring: The index of the state as a bitstring.

        Returns:
            The complex amplitude of the state.
        """

    def get_classical_variable(self, name: str) -> Variable:
        """Gets a classical variable by name.

        For registers, the name should be the register name followed by the index
        in square brackets.

        Args:
            name: The name of the variable.

        Returns:
            The fetched variable.
        """

    def get_num_classical_variables(self) -> int:
        """Gets the number of classical variables in the simulation.

        For registers, each index is counted as a separate variable.

        Returns:
            The number of classical variables in the simulation.
        """

    def get_classical_variable_name(self, index: int) -> str:
        """Gets the name of a classical variable by its index.

        For registers, each index is counted as a separate variable and can be
        accessed separately. This method will return the name of the specific
        index of the register.

        Args:
            index: The index of the variable.

        Returns:
            The name of the variable.
        """

    def get_quantum_variable_name(self, index: int) -> str:
        """Gets the name of a quantum variable by its index.

        For registers, each index is counted as a separate variable and can be
        accessed separately. This method will return the name of the specific
        index of the register.

        Args:
            index: The index of the variable.

        Returns:
            The name of the variable.
        """

    def get_state_vector_full(self) -> Statevector:
        """Gets the full state vector of the simulation at the current time.

        Returns:
            The full state vector of the current simulation state.
        """

    def get_state_vector_sub(self, qubits: Sequence[int]) -> Statevector:
        """Gets a sub-state of the state vector of the simulation at the current time.

        This method also supports the re-ordering of qubits, but does not allow
        qubits to be repeated.

        Args:
            qubits: The qubits to include in the sub-state.

        Returns:
            The sub-state vector of the current simulation state.
        """

    def set_breakpoint(self, desired_position: int) -> int:
        """Sets a breakpoint at the desired position in the code.

        The position is given as a 0-indexed character position in the full code
        string.

        Args:
            desired_position: The position in the code to set the breakpoint.

        Returns:
            The index of the instruction where the breakpoint was set.
        """

    def clear_breakpoints(self) -> None:
        """Clears all breakpoints set in the simulation."""

    def get_stack_depth(self) -> int:
        """Gets the current stack depth of the simulation.

        Each custom gate call corresponds to one stack entry.

        Returns:
            The current stack depth of the simulation.
        """

    def get_stack_trace(self, max_depth: int) -> list[int]:
        """Gets the current stack trace of the simulation.

        The stack trace is represented as a list of instruction indices. Each
        instruction index represents a single return address for the corresponding
        stack entry.

        Args:
            max_depth: The maximum depth of the stack trace.

        Returns:
            The stack trace of the simulation.
        """

    def get_diagnostics(self) -> Diagnostics:
        """Gets the diagnostics instance employed by this debugger.

        Returns:
            The diagnostics instance employed by this debugger.
        """

    def compile(self, settings: CompilationSettings) -> str:
        """Compiles the given code into a quantum circuit without assertions.

        Args:
            settings: The settings to use for the compilation.

        Returns:
            The compiled code.
        """

def create_ddsim_simulation_state() -> SimulationState:
    """Creates a new `SimulationState` instance using the DD backend for simulation and the OpenQASM language as input format.

    Returns:
        The created simulation state.
    """

def destroy_ddsim_simulation_state(state: SimulationState) -> None:
    """Delete a given DD-based `SimulationState` instance and free up resources.

    Args:
        state: The simulation state to delete.
    """
