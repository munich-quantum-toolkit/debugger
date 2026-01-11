# Copyright (c) 2024 - 2026 Chair for Design Automation, TUM
# Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""This module tests the end-to-end functionality of the simulator by running a set of QASM files and comparing their results with the expected ones."""

from __future__ import annotations

import locale
from dataclasses import dataclass
from math import log2
from pathlib import Path
from typing import TYPE_CHECKING

import pytest

from mqt.debugger import Complex, Statevector, create_ddsim_simulation_state
from mqt.debugger.pydebugger import destroy_ddsim_simulation_state

if TYPE_CHECKING:
    from mqt.debugger import SimulationState


@dataclass
class ExpectedSolution:
    """Represents the expected solution of a quantum program."""

    statevector: Statevector
    classical: dict[str, bool]
    failed_assertions: int


def parse_complex(text: str) -> Complex:
    """Parses a complex number from a string.

    Args:
        text (str): The string to parse.

    Returns:
        Complex: The complex number represented by the string.
    """
    c = complex(text)
    return Complex(c.real, c.imag)


def parse_solution(text: str) -> ExpectedSolution:
    """Parses the expected solution of a quantum program from a string.

    Args:
        text (str): The string to parse.

    Returns:
        ExpectedSolution: The expected solution of the quantum program.
    """
    parts = text.replace("\n", " ").replace("  ", " ").split("---")
    amplitudes = [parse_complex(x) for x in parts[0].split(",")]
    sv = Statevector()
    sv.amplitudes = amplitudes
    sv.num_qubits = int(log2(len(amplitudes)))
    sv.num_states = len(amplitudes)

    classical_values = parts[1].strip().split(",") if parts[1].strip() else []
    classical_dict: dict[str, bool] = {}
    for value in classical_values:
        key, val = value.split("=")
        classical_dict[key.strip()] = val.strip() == "1"

    failed_assertions = int(parts[2].strip())

    return ExpectedSolution(statevector=sv, classical=classical_dict, failed_assertions=failed_assertions)


def load_instance(name: str) -> tuple[SimulationState, ExpectedSolution]:
    """Loads a quantum simulation and debugging instance from a file.

    Args:
        name (str): The name of the instance to load.

    Returns:
        tuple[SimulationState, ExpectedSolution]: The simulation state and the expected solution.
    """
    state = create_ddsim_simulation_state()
    code = Path(f"test/python/resources/end-to-end/{name}.qasm").read_text(encoding=locale.getpreferredencoding(False))
    state.load_code(code)
    text = Path(f"test/python/resources/end-to-end/{name}.out").read_text(encoding=locale.getpreferredencoding(False))
    solution = parse_solution(text)
    return (state, solution)


def complex_approximately_equal(a: Complex, b: Complex, epsilon: float = 1e-2) -> bool:
    """Checks if two complex numbers are approximately equal.

    Args:
        a (Complex): The first complex number.
        b (Complex): The second complex number.
        epsilon (float, optional): The comparison threshold. Defaults to 1e-2.

    Returns:
        bool: True if the numbers are approximately equal, False otherwise.
    """
    return abs(a.real - b.real) < epsilon and abs(a.imaginary - b.imaginary) < epsilon


def assert_statevectors_equal(result: Statevector, expected: Statevector) -> None:
    """Asserts that two statevectors are equal.

    Args:
        result (Statevector): The statevector to check.
        expected (Statevector): The expected statevector.
    """
    assert result.num_qubits == expected.num_qubits, f"Expected {expected.num_qubits} qubits, got {result.num_qubits}"
    assert result.num_states == expected.num_states, (
        f"Expected {expected.num_states} amplitudes, got {result.num_states}"
    )
    for i in range(result.num_states):
        assert complex_approximately_equal(result.amplitudes[i], expected.amplitudes[i]), (
            f"Expected amplitude {expected.amplitudes[i]} at index {i}, got {result.amplitudes[i]}"
        )


@pytest.mark.parametrize(
    "instance",
    [
        "bell",
        "ghz_3",
        "ghz_5",
        "grover_3",
        "qpe",
        "dj_4",
        "bv",
        "fail_ghz",
        "fail_eq",
    ],
)
def test_end_to_end(instance: str) -> None:
    """Test multiple quantum programs end-to-end.

    Args:
        instance (str): The name of the instance to test.
    """
    state, solution = load_instance(instance)
    errors = state.run_all()
    assert errors == solution.failed_assertions, (
        f"Expected {solution.failed_assertions} failed assertions, got {errors}"
    )
    assert_statevectors_equal(state.get_state_vector_full(), solution.statevector)

    for key, value in solution.classical.items():
        try:
            result_value = state.get_classical_variable(key).value.bool_value
        except RuntimeError:
            msg = f"Expected classical value {key} to be {value}, but it was not found"
            raise AssertionError(msg) from None
        assert value == result_value, f"Expected classical value {key} to be {value}, got {result_value}"
    destroy_ddsim_simulation_state(state)
