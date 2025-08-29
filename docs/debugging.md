# Interactive Debugging

MQT Debugger provides various methods to interactively debug quantum programs.
It can be used to step through the program and inspect the state of the system.
This document further details the debugging capabilities provided by this framework.

As MQT Debugger currently only supports OpenQASM as input language, this document will give examples and details related to OpenQASM programs.
However, due to the modular nature of this framework, it can be extended to support other languages as well.

## Stepping Through the Program

The most basic debugging feature is stepping through the program.
Like many typical interactive debuggers, this framework supports three types of steps:

- **Single Step**: Executes the next instruction and stops afterwards. If the next instruction is a function call, it will step into the function.
- **Step Over**: Executes the next instruction and stops afterwards. If the next instruction is a function call, it will execute the entire function at once.
- **Step Out**: Continues execution until the current function returns.

However, due to the reversibility of quantum programs, the debugger can also step backward:

- **Single Step Backward**: Reverts the last instruction and stops afterwards. If the last instruction was a function call, it will only stop at the last instruction of that function.
- **Step Over Backward**: Reverts the last instruction and stops afterwards. If the last instruction was a function call, it will revert the entire function at once.
- **Step Out Backward**: Reverts all instructions in the current function scope and stops at the instruction calling the function of the current scope.

## Continuing and Pausing Execution

In contrast to individual steps, the debugger can also be used to continue the entire program execution, using the {cpp:member}`SimulationState::runSimulation <SimulationStateStruct::runSimulation>`/{py:meth}`SimulationState.run_simulation <mqt.debugger.SimulationState.run_simulation>` method.
This will execute the program until the end or until a {ref}`breakpoint <breakpoints>` or a {ref}`failing assertion <assertions>` is reached.

Additionally, the {cpp:member}`SimulationState::runAll <SimulationStateStruct::runAll>`/{py:meth}`SimulationState.run_all <mqt.debugger.SimulationState.run_all>` method can be used to run the program without stopping, instead counting how many failing assertions were encountered.

Furthermore, the {cpp:member}`SimulationState::pauseSimulation <SimulationStateStruct::pauseSimulation>`/{py:meth}`SimulationState.pause_simulation <mqt.debugger.SimulationState.pause_simulation>` method can be used to pause the execution at any point in time.

## Inspecting the State

MQT Debugger distinguishes between classical variables and quantum variables. For OpenQASM, currently only boolean classical variables are supported.

The framework provides different methods to inspect the state of the system at runtime. Classical variables can be accessed using the {cpp:member}`SimulationState::getClassicalVariable <SimulationStateStruct::getClassicalVariable>`/{py:meth}`SimulationState.get_classical_variable <mqt.debugger.SimulationState.get_classical_variable>` method, passing the name of the desired variable, which returns an object representing the variable.

Quantum variables cannot be accessed directly, but the developer can instead access the statevector to inspect the quantum state at any point in time. {cpp:member}`SimulationState::getStateVectorFull <SimulationStateStruct::getStateVectorFull>`/{py:meth}`SimulationState.get_state_vector_full <mqt.debugger.SimulationState.get_state_vector_full>` can be used to obtain the full statevector of the system. As this statevector can be very large, {cpp:member}`SimulationState::getStateVectorSub <SimulationStateStruct::getStateVectorSub>`/{py:meth}`SimulationState.get_state_vector_sub <mqt.debugger.SimulationState.get_state_vector_sub>` can be used to obtain a sub-statevector of the system, containing just a subset of all qubits. In this case, the qubits included in the sub-statevector must not be entangled with any qubits outside it.

Furthermore, the framework also allows to inspect individual amplitude values of the statevector using {cpp:member}`SimulationState::getAmplitudeIndex <SimulationStateStruct::getAmplitudeIndex>`/{py:meth}`SimulationState.get_amplitude_index <mqt.debugger.SimulationState.get_amplitude_index>` or {cpp:member}`SimulationState::getAmplitudeBitstring <SimulationStateStruct::getAmplitudeBitstring>`/{py:meth}`SimulationState.get_amplitude_bitstring <mqt.debugger.SimulationState.get_amplitude_bitstring>`. In these cases, the developer must identify the desired amplitude by passing either the index of the amplitude or the bitstring that represents the desired state.

## Breakpoints

Breakpoints can be set to force execution to stop at a specific instruction. To be compatible with the typical protocols, setting a breakpoint requires the character index in the source code, at which the breakpoint should be set. MQT Debugger will then determine the instruction that corresponds to this location in the code and stop execution there in the future.

To set a breakpoint, the {cpp:member}`SimulationState::setBreakpoint <SimulationStateStruct::setBreakpoint>`/{py:meth}`SimulationState.set_breakpoint <mqt.debugger.SimulationState.set_breakpoint>` method can be used, passing the desired character index. This method will return the instruction index at which the breakpoint was set (Python) or store it in the provided reference (C++).

To remove breakpoints, the {cpp:member}`SimulationState::clearBreakpoints <SimulationStateStruct::clearBreakpoints>`/{py:meth}`SimulationState.clear_breakpoints <mqt.debugger.SimulationState.clear_breakpoints>` method can be used, removing all breakpoints.

When a program is paused during execution, the methods {cpp:member}`SimulationState::wasBreakpointHit <SimulationStateStruct::wasBreakpointHit>`/{py:meth}`SimulationState.was_breakpoint_hit <mqt.debugger.SimulationState.was_breakpoint_hit>` can be used to check whether the current pause was caused by a breakpoint. This allows developers to distinguish between pauses due to breakpoints and pauses due to other reasons, such as {ref}`failed assertions <assertions>`.

## Assertions

MQT Debugger supports {doc}`assertions <Assertions>` to check the state of the system at runtime.
If an assertion fails, the debugger will pause execution **before** the failing instruction.

Any methods to {ref}`step through the program <stepping>` will pause at a failing assertion. Continuing execution after hitting a failed assertion will skip the failing instruction and continue with the next one.

When a program is paused during execution, the methods {cpp:member}`SimulationState::didAssertionFail <SimulationStateStruct::didAssertionFail>`/{py:meth}`SimulationState.did_assertion_fail <mqt.debugger.SimulationState.did_assertion_fail>` can be used to check whether the current pause was caused by a failing assertion. This allows developers to distinguish between pauses due to assertions and pauses due to other reasons, such as {ref}`breakpoints <breakpoints>`.
