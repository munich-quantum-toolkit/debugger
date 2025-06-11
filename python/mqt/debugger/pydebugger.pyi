# Copyright (c) 2024 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Type stubs for python bindings of the debug module."""

import enum

# Enums

class VariableType(enum.Enum):
    """Represents possible types of classical variables."""

    VarBool = 0
    """A boolean variable."""
    VarInt = 1
    """A 32-bit integer variable."""
    VarFloat = 2
    """A floating-point variable."""

# Classes
class VariableValue:
    """Represents the value of a classical variable.

    Only one of these fields has a valid value at a time, based on the variable's `VariableType`.
    """

    bool_value: bool
    """The value of a boolean variable."""
    int_value: int
    """The value of a 32-bit integer variable."""
    float_value: float
    """The value of a floating-point variable."""

    def __init__(self) -> None:
        """Creates a new `VariableValue` instance."""

class Variable:
    """Represents a classical variable."""

    name: str
    """The name of the variable."""
    type: VariableType
    """The type of the variable."""
    value: VariableValue
    """The value of the variable."""

    def __init__(self) -> None:
        """Creates a new `Variable` instance."""

class Complex:
    """Represents a complex number."""

    real: float
    """The real part of the complex number."""
    imaginary: float
    """The imaginary part of the complex number."""

    def __init__(self, real: float = 0.0, imaginary: float = 0.0) -> None:
        """Initializes a new complex number.

        Args:
            real (float, optional): The real part of the complex number. Defaults to 0.0.
            imaginary (float, optional): The imaginary part of the complex number. Defaults to 0.0.
        """

class CompilationSettings:
    """The settings that should be used to compile an assertion program."""

    opt: int
    """The optimization level that should be used. Exact meaning depends on the implementation, but typically 0 means no optimization."""
    slice_index: int
    """The index of the slice that should be compiled."""

    def __init__(self, opt: int, slice_index: int = 0) -> None:
        """Initializes a new set of compilation settings.

        Args:
            opt (int): The optimization level that should be used.
            slice_index (int, optional): The index of the slice that should be compiled (defaults to 0).
        """

class Statevector:
    """Represents a state vector."""

    num_qubits: int
    """The number of qubits in the state vector."""
    num_states: int
    """The number of states in the state vector.

    This is always equal to 2^`num_qubits`.
    """

    amplitudes: list[Complex]
    """The amplitudes of the state vector.

    Contains one element for each of the `num_states` states in the state vector.
    """

    def __init__(self) -> None:
        """Creates a new `Statevector` instance."""

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
            code (str): The code to load.
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
        int: The number of assertions that failed during execution.
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
        bool: True, if the simulation can step forward.
        """

    def can_step_backward(self) -> bool:
        """Indicates whether the simulation can step backward.

        The simulation is unable to step backward if it is at the beginning or if
        the simulation has not been set up yet.

        Returns:
        bool: True, if the simulation can step backward.
        """

    def is_finished(self) -> bool:
        """Indicates whether the execution has finished.

        The execution is considered finished if it has reached the end of the code.

        Returns:
        bool: True, if the execution has finished.
        """

    def did_assertion_fail(self) -> bool:
        """Indicates whether an assertion has failed in the previous step.

        If execution is continued after a failed assertion, then this flag will
        be set to false again.

        Returns:
        bool: True, if an assertion failed during the last step.
        """

    def was_breakpoint_hit(self) -> bool:
        """Indicates whether a breakpoint was hit in the previous step.

        If execution is continued after a breakpoint, then this flag will
        be set to false again.

        Returns:
        bool: True, if a breakpoint was hit during the last step.
        """

    def get_current_instruction(self) -> int:
        """Gets the current instruction index.

        Returns:
        int: The index of the current instruction.
        """

    def get_instruction_count(self) -> int:
        """Gets the number of instructions in the code.

        Returns:
            int: The number of instructions in the code.
        """

    def get_instruction_position(self, instruction: int) -> tuple[int, int]:
        """Gets the position of the given instruction in the code.

        Start and end positions are inclusive and white-spaces are ignored.

        Args:
            instruction (int): The instruction to find.

        Returns:
            tuple[int, int]: The start and end positions of the instruction.
        """

    def get_num_qubits(self) -> int:
        """Gets the number of qubits used by the program.

        Returns:
            int: The number of qubits used by the program.
        """

    def get_amplitude_index(self, index: int) -> Complex:
        """Gets the complex amplitude of a state in the full state vector.

        The amplitude is selected by an integer index that corresponds to the
        binary representation of the state.

        Args:
            index (int): The index of the state in the full state vector.

        Returns:
            Complex: The complex amplitude of the state.
        """

    def get_amplitude_bitstring(self, bitstring: str) -> Complex:
        """Gets the complex amplitude of a state in the full state vector.

        The amplitude is selected by a bitstring representing the state.

        Args:
            bitstring (str): The index of the state as a bitstring.

        Returns:
            Complex: The complex amplitude of the state.
        """

    def get_classical_variable(self, name: str) -> Variable:
        """Gets a classical variable by name.

        For registers, the name should be the register name followed by the index
        in square brackets.

        Args:
            name (str): The name of the variable.

        Returns:
            Variable: The fetched variable.
        """

    def get_num_classical_variables(self) -> int:
        """Gets the number of classical variables in the simulation.

        For registers, each index is counted as a separate variable.

        Returns:
            int: The number of classical variables in the simulation.
        """

    def get_classical_variable_name(self, index: int) -> str:
        """Gets the name of a classical variable by its index.

        For registers, each index is counted as a separate variable and can be
        accessed separately. This method will return the name of the specific
        index of the register.

        Args:
            index (int): The index of the variable.

        Returns:
            str: The name of the variable.
        """

    def get_state_vector_full(self) -> Statevector:
        """Gets the full state vector of the simulation at the current time.

        The state vector is expected to be initialized with the correct number of
        qubits and allocated space for the amplitudes before calling this method.

        Returns:
            Statevector: The full state vector of the current simulation state.
        """

    def get_state_vector_sub(self, qubits: list[int]) -> Statevector:
        """Gets a sub-state of the state vector of the simulation at the current time.

        The state vector is expected to be initialized with the correct number of
        qubits and allocated space for the amplitudes before calling this method.

        This method also supports the re-ordering of qubits, but does not allow
        qubits to be repeated.

        Args:
            qubits (list[int]): The qubits to include in the sub-state.

        Returns:
            Statevector: The sub-state vector of the current simulation state.
        """

    def set_breakpoint(self, desired_position: int) -> int:
        """Sets a breakpoint at the desired position in the code.

        The position is given as a 0-indexed character position in the full code
        string.

        Args:
            desired_position (int): The position in the code to set the breakpoint.

        Returns:
            int: The index of the instruction where the breakpoint was set.
        """

    def clear_breakpoints(self) -> None:
        """Clears all breakpoints set in the simulation."""

    def get_stack_depth(self) -> int:
        """Gets the current stack depth of the simulation.

        Each custom gate call corresponds to one stack entry.

        Returns:
            int: The current stack depth of the simulation.
        """

    def get_stack_trace(self, max_depth: int) -> list[int]:
        """Gets the current stack trace of the simulation.

        The stack trace is represented as a list of instruction indices. Each
        instruction index represents a single return address for the corresponding
        stack entry.

        Args:
            max_depth (int): The maximum depth of the stack trace.

        Returns:
            list[int]: The stack trace of the simulation.
        """

    def get_diagnostics(self) -> Diagnostics:
        """Gets the diagnostics instance employed by this debugger.

        Returns:
            Diagnostics: The diagnostics instance employed by this debugger.
        """

    def compile(self, settings: CompilationSettings) -> str:
        """Compiles the program in the current state.

        Args:
        settings (CompilationSettings): The settings to use for the compilation.

        Returns:
            str: The compiled code.
        """

class ErrorCauseType(enum.Enum):
    """Represents the type of a potential error cause."""

    Unknown = 0
    """An unknown error cause."""
    MissingInteraction = 1
    """Indicates that an entanglement error may be caused by a missing interaction."""
    ControlAlwaysZero = 2
    """Indicates that an error may be related to a controlled gate with a control that is always zero."""

class ErrorCause:
    """Represents a potential cause of an assertion error."""

    instruction: int
    """The instruction where the error may originate from or where the error can be detected."""
    type: ErrorCauseType
    """The type of the potential error cause."""

    def __init__(self) -> None:
        """Creates a new `ErrorCause` instance."""

class Diagnostics:
    """Provides diagnostics capabilities such as different analysis methods for the debugger."""
    def __init__(self) -> None:
        """Creates a new `Diagnostics` instance."""

    def init(self) -> None:
        """Initializes the diagnostics instance."""

    def get_num_qubits(self) -> int:
        """Get the number of qubits in the system.

        Returns:
            int: The number of qubits in the system.
        """

    def get_instruction_count(self) -> int:
        """Get the number of instructions in the code.

        Returns:
            int: The number of instructions in the code.
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
            instruction (int): The instruction to extract the data dependencies for.
            include_callers (bool, optional): True, if the data dependencies should include all possible callers of the containing custom gate. Defaults to False.

        Returns:
            list[int]: A list of instruction indices that are data dependencies of the given instruction.
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
            before_instruction (int): The instruction to extract the interactions up to (excluding).
            qubit (int): The qubit to extract the interactions for.

        Returns:
            list[int]: A list of qubit indices that interact with the given qubit up to the target instruction.
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
            list[int]: The indices of instructions that are controlled gates with only zero controls.
        """

    def potential_error_causes(self) -> list[ErrorCause]:
        """Extract a list of potential error causes encountered during execution.

        This method should be run after the program has been executed and reached
        an assertion error.

        Returns:
            list[ErrorCause]: A list of potential error causes encountered during execution.
        """

    def suggest_assertion_movements(self) -> tuple[int, int]:
        """Suggest movements of assertions to better positions.

        Each entry of the resulting list consists of the original position of the assertion, followed by its new
        suggested position.

        Returns:
            list[tuple[int, int]]: A list of moved assertions.
        """

    def suggest_new_assertions(self) -> tuple[int, str]:
        """Suggest new assertions to be added to the program.

        Each entry of the resulting list consists of the suggested position for the new assertion, followed by its
        string representation.

        Returns:
          list[tupke[int, str]]: A list of new assertions.
        """

def create_ddsim_simulation_state() -> SimulationState:
    """Creates a new `SimulationState` instance using the DD backend for simulation and the OpenQASM language as input format.

    Returns:
        SimulationState: The created simulation state.
    """

def destroy_ddsim_simulation_state(state: SimulationState) -> None:
    """Delete a given DD-based `SimulationState` instance and free up resources.

    Args:
        state (SimulationState): The simulation state to delete.
    """
