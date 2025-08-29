# Assertion Refinement

This document describes the assertion refinement methods available in MQT Debugger.
Assertion refinement can be used to improve the quality of existing {doc}`assertions <Assertions>` in the program by either moving them to earlier positions or creating new assertions that are easier to check.

All of these methods require a {py:class}`Diagnostics <mqt.debugger.Diagnostics>` object, which can be obtained from the {py:class}`SimulationState <mqt.debugger.SimulationState>` object using {cpp:member}`SimulationState::getDiagnostics <SimulationStateStruct::getDiagnostics>`/{py:meth}`SimulationState.get_diagnostics <mqt.debugger.SimulationState.get_diagnostics>`.

For further information, please refer to {cite}`rovara2024assertionrefinement`.

## Assertion Movement

The process of assertion movement utilizes commutation rules to move assertions to earlier positions in the program.
Assertions that are moved to earlier positions are closer to the error cause and therefore reduce the manual debugging workload for the developer and lower the likelihood of other diagnosis methods to yield inconclusive results.

A list of assertions that can be moved can be obtained using {cpp:member}`Diagnostics::suggestAssertionMovements <DiagnosticsStruct::suggestAssertionMovements>`/{py:meth}`Diagnostics.suggest_assertion_movements <mqt.debugger.Diagnostics.suggest_assertion_movements>`.
In the Python version, the method returns a list of pairs where each pair consists of the index of the assertion that can be moved and the index of the instruction to which it can be moved.
In the C++ version, pointers to an array of starting positions and an array of target positions must be passed to the method. It will then fill these arrays with the corresponding indices.
In either case, this approach will attempt to move all assertions as much as possible.

All assertions commute with instructions that do not use any of their inspected qubits as controls or targets. However, additional rules can be applied to different assertion types to move them over more instructions:

### Moving Superposition Assertions

Superposition assertions can be moved over all instructions that can be represented as diagonal or anti-diagonal matrices.
In this example:

```qasm
qreg q[3];

h q[0];
x q[0];
cz q[0], q[1];
x q[0];

assert-sup q[0];
```

the superposition assertion can be moved over both the {code}`x` and {code}`cz` gates, as they are diagonal matrices.
The {code}`h` gate, however, is not diagonal and therefore blocks the assertion from being moved over it, resulting in the following code:

```qasm
qreg q[3];

h q[0];
assert-sup q[0];
x q[0];
cz q[0], q[1];
x q[0];
```

### Moving Entanglement Assertions

Entanglement assertions can be moved over all single-qubit instructions.
In this example:

```qasm
qreg q[3];

h q[0];
cx q[0], q[1];
cz q[0], q[1];
h q[0];
x q[0];

assert-ent q[0], q[1];
```

the entanglement assertion can only be moved up to the {code}`cz` gate, resulting in the following code:

```qasm
qreg q[3];

h q[0];
cx q[0], q[1];
cz q[0], q[1];
assert-ent q[0], q[1];
h q[0];
x q[0];
```

## Assertion Creation

Assertion creation tries to create new assertions from existing "ground-truth" assertions in the program. This allows developers to write simpler assertions that can be made more precise by the debugging framework, if necessary.

A list of assertions that can be created can be obtained using {cpp:member}`Diagnostics::suggestNewAssertions <DiagnosticsStruct::suggestNewAssertions>`/{py:meth}`Diagnostics.suggest_new_assertions <mqt.debugger.Diagnostics.suggest_new_assertions>`.
In the Python version, this method returns a list of pairs where each pair consists of the index of the new assertion that can be added and a string representation of the new assertion.
In the C++ version, pointers to an array of positions and an array of assertion string-representations must be passed to the method. It will then fill these arrays with the corresponding values.

Two types of new assertions can be created:

### Creating Equality Assertions

Existing equality assertions can be split into several smaller assertions automatically. This allows fractions of assertions to be pulled (using {ref}`assertion movement <assertion_movement>`) over other instructions that may otherwise block them.

Equality assertions are split apart by taking their state vector representation and attempting to split it into separable sub states. These sub states are then used to create new assertions. The following code shows an example situation, in which Assertion Creation would find a potential new assertion:

```qasm
qreg q[3];

h q[0];
cx q[0], q[1];
h q[2];

assert-eq q { 0.5, 0, 0, 0.5, 0.5, 0, 0, 0.5 }
```

Clearly, {code}`q[2]` is separable from the other two qubits, resulting in new assertions:

```qasm
qreg q[3];

h q[0];
cx q[0], q[1];
h q[2];

assert-eq 0.999, q[0], q[1] { 0.70711, 0, 0, 0.70711 }
assert-eq 0.999, q[2] { 0.70711, 0.70711 }
```

```{note}
The amplitudes of the split states now have a value of $\frac{1}{\sqrt{2}}$. As this cannot be expressed by a rational number, the amplitudes are rounded and a similarity threshold of 0.999 is introduced.
```

### Creating Entanglement Assertions

By inspecting previous interactions of qubits, new entanglement assertions can be created from existing ones. This is done by finding other qubits that may be entangled with the target qubits of the existing assertion and creating new assertions for them.

The following code shows an example situation, in which new entanglement assertions would be created:

```qasm
qreg q[3];

h q[0];
cx q[1], q[0];
cx q[1], q[2];

assert-ent q[0], q[2];
```

Here, the interactions between {code}`q[0]` and {code}`q[2]` suggest that {code}`q[1]` must be entangled with them for the assertion to be satisfied. Therefore, we introduce new assertions:

```qasm
qreg q[3];

h q[0];
cx q[1], q[0];
cx q[1], q[2];

assert-ent q[0], q[1];
assert-ent q[1], q[2];
```
