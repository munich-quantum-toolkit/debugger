# Assertions

This document describes the syntax and semantics of assertions in MQT Debugger.
Assertions are a useful tool to test the correctness of programs by comparing the current state of the system to an expected state.
The assertions in MQT Debugger allow developers to test either the exact or approximate state or certain properties, such as entanglement or superposition.

The following sections will give an overview of the assertion syntax and the different types of assertions that can be used in MQT Debugger.

```{note}
The targets of an assertion can be either individual qubits or full quantum registers. However, when the selected targets are a subset of the full quantum state, then none of the qubits in this sub-state is allowed to be entangled with any other qubits outside of the sub-state. This is because, in many cases, exact sub-states as state vectors are not well-defined if they are entangled with other qubits.
```

## Entanglement Assertion

Entanglement assertions check whether a set of qubits is entangled. For this purpose, every qubit in the set is compared to all other qubits in the set, and entanglement must exist between each possible pair.

**QASM Syntax**:

```qasm
assert-ent <target_qubit_1> [, <target_qubit_2> ...]*;
```

_Example_:

```qasm
assert-ent q[0], q[1], q[2];
```

This assertion checks for entanglement between qubits {code}`q[0]` and {code}`q[1]`, {code}`q[0]` and {code}`q[2]`, as well as {code}`q[1]` and {code}`q[2]`.

An example for a quantum state that would pass this assertion is the GHZ state {math}`\frac{1}{\sqrt{2}}(|000\rangle + |111\rangle)`.
As none of the individual qubits is separable, this state is entangled.

An example for a quantum state that would fail this assertion is the state {math}`\frac{1}{\sqrt{2}}(|000\rangle + |110\rangle)`.
As the least-significant qubit is separable, this state is not entangled.

## Superposition Assertion

Superposition assertions check whether a set of qubits is in a superposition state.
A set of qubits is in a superposition state if there exist at least two basis states of the full system that have different assignments for the target qubits and a non-zero amplitude.
This means that not every qubit in the set must be in a superposition state individually, but the full set must be in a superposition state.

**QASM Syntax**:

```qasm
assert-sup <target_qubit_1> [, <target_qubit_2> ...]*;
```

_Example_:

```qasm
assert-sup q[0], q[1], q[2];
```

This assertion checks for superposition of qubits {code}`q[0]`, {code}`q[1]`, and {code}`q[2]`.

An example for a quantum state that would pass this assertion is the state {math}`\frac{1}{\sqrt{2}}(|0\rangle + |1\rangle) \otimes |0\rangle`.
As two basis states have non-zero amplitudes ({math}`|00\rangle` and {math}`|10\rangle`), this state is in a superposition.

An example for a quantum state that would fail this assertion is the state {math}`|00\rangle`.
In this case, only a single state ({math}`|00\rangle`) has a non-zero amplitude, so the state is not in a superposition.

## Equality Assertion

Equality assertions compare the state of a set of qubits to a given state and fail if the states are not equal.
Furthermore, a similarity threshold can be passed to the assertion, allowing for approximate comparisons. The similarity is computed through the cosine similarity of two functions and can be set between 0 and 1. If no similarity threshold is passed, the default value of 1 is used.

The target state to compare to can be expressed as a state vector or as a new quantum circuit.

**QASM Syntax**:

```qasm
assert-eq [similarity], <target_qubit_1> [, <target_qubit_2> ...]* { STATE_REPRESENTATION }
```

```qasm
STATE_REPRESENTATION =
    | <STATEVECTOR>
    | <CIRCUIT>

STATEVECTOR = <amplitude_1> [, <amplitude_2> ...]*
CIRCUIT = <QASM_CODE>
```

If the selected state representation is a state vector, it must represent a system of the same number of qubits as the assertion compares to and use $2^n$ amplitudes. Amplitudes can be real or complex numbers using $i$ or $j$ for the imaginary unit.

If the selected state representation is a circuit, it must be defined as a new quantum program in QASM format. The circuit must not contain any further assertions and must use the same number of qubits as the assertion compares to. The current system state will then be compared to the state after running the circuit.

_Example_:

```qasm
assert-eq 0.9, q[0], q[1] { 0.5, 0.5, 0.5, 0.5 };
```

This assertion checks whether the state of qubits {code}`q[0]` and {code}`q[1]` is equal to the state {math}`\frac{1}{2}(|00\rangle + |01\rangle + |10\rangle + |11\rangle)` with a similarity threshold of 0.9.

```qasm
assert-eq q[0], q[1] {
    h q[0];
    cx q[0], q[1];
};
```

This assertion checks whether the state of qubits {code}`q[0]` and {code}`q[1]` is equal to the bell state {math}`\frac{1}{\sqrt{2}}(|00\rangle + |11\rangle)`.
