# Diagnosis Methods

This document describes the diagnostics methods available in MQT Debugger.
The diagnostics methods can be used to analyze the state of the system, in particular to find potential error causes when an {doc}`assertion <Assertions>` fails.

All of these methods require a {py:class}`Diagnostics <mqt.debugger.Diagnostics>` object to be executed, which can be obtained from the {py:class}`SimulationState <mqt.debugger.SimulationState>` object using {cpp:member}`SimulationState::getDiagnostics <SimulationStateStruct::getDiagnostics>`/{py:meth}`SimulationState.get_diagnostics <mqt.debugger.SimulationState.get_diagnostics>`.

## Error Cause Analysis

When an assertion fails, several methods can be used to find potential error causes.

For further information, please refer to {cite:p}`rovara2024debugging`.

### Cone of Influence Analysis

Cone of Influence Analysis partitions a given quantum program into two parts: the part that influences the failed assertion and the part that does not.
This analysis method can be used to reduce the number of instructions that need to be checked for potential error causes. If the Cone of Influence analysis manages to narrow down just a small subset of the program, the error cause can be found more quickly. Furthermore, other analysis methods use the Cone of Influence as input to find potential error causes more efficiently.

The cone of influence can be obtained using {cpp:member}`Diagnostics::getDataDependencies <DiagnosticsStruct::getDataDependencies>`/{py:meth}`Diagnostics.get_data_dependencies <mqt.debugger.Diagnostics.get_data_dependencies>`.
In the Python version, it requires a single instruction to be passed as an argument, and returns the indices of all instructions that influence it.
In the C++ version, in addition to the desired instruction, a pointer to a boolean array must be passed. Each element of the array corresponds to an instruction in the program and is set to {code}`true` if the instruction is part of the cone of influence.
Finally, a boolean flag (`includeCallers`) can be passed to also include instructions outside the current scope in the cone of influence. Otherwise, if the function is called inside a custom gate definition, it will only include data dependencies within the gate.

The cone of influence is computed by recursively iterating over the instructions in the current cone of influence and adding all instructions that influence them, until it reaches a fixed point.

### Interaction Analysis

Interaction Analysis is a method to find potential error causes by analyzing the interactions between qubits in the system.
It is automatically called when using {cpp:member}`Diagnostics::potentialErrorCauses <DiagnosticsStruct::potentialErrorCauses>`/{py:meth}`Diagnostics.potential_error_causes <mqt.debugger.Diagnostics.potential_error_causes>`.

This analysis method can be used to find reasons for failing entanglement assertions:
Whenever a failed entanglement assertion is encountered, the Interaction Analysis checks whether the target qubits of the assertion interact with each other.

If this is not the case, then clearly, no entanglement can be prepared, so it is treated as a potential error cause. The following code shows an example situation in which Interaction Analysis would find a potential error cause:

```qasm
qreg q[3];

h q[0];
cx q[0], q[1];

assert-ent q;
```

Here, calling {code}`potential_error_causes()` yields two errors: {code}`Missing interaction between q[0] and q[2]` and {code}`Missing interaction between q[1] and q[2]`.

```{note}
Interaction Analysis is generally a static analysis method.
However, when it is performed at runtime for the {code}`potential_error_causes()` method, it further uses dynamically obtained information to improve its results.
During execution, the diagnostics tool keeps track of all actual qubits that were involved in instructions, even inside custom gate definitions, where static analysis would not be able to determine the exact qubits involved.
This way, it can extend the interaction analysis throughout the entire program, even in other scopes.
This is not always possible when performing interaction analysis statically.
```

## Control-Value Analysis

Control-Value Analysis is a method that dynamically analyzes the program during execution to find incorrectly defined controlled gates.
In particular, it looks for controlled gates for which the control is always purely in the state {math}`|0\rangle`. In these cases,
the controlled gate will never affect the full state, which could be a sign for an error.

This analysis also similarly checks for inverse-controlled gates (i.e., controlled gates that trigger when the control value is {math}`|1\rangle`) that always
have the state {math}`|0\rangle` as control.

It is automatically called when using {cpp:member}`Diagnostics::potentialErrorCauses <DiagnosticsStruct::potentialErrorCauses>`/{py:meth}`Diagnostics.potential_error_causes <mqt.debugger.Diagnostics.potential_error_causes>`.

The following code shows an example situation, in which Control-Value Analysis would find a potential error cause:

```qasm
qreg q[3];

h q[0];
cx q[0], q[1];
cx q[2], q[0];

assert-ent q;
```

Here, calling {code}`potential_error_causes()` yields the error {code}`Controlled gate with constant control value for instruction cx q[2], q[0]`.
