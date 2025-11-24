# Copyright (c) 2024 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""This module contains tests for the Python bindings of the Debugging interface.

It only tests whether the bindings are working correctly. It does not stress-test the functionality or test edge cases.
"""

from __future__ import annotations

import locale
from pathlib import Path
import math
from typing import TYPE_CHECKING, cast

import pytest

import mqt.debugger

if TYPE_CHECKING:
    from collections.abc import Generator

SimulationInstance = tuple[mqt.debugger.SimulationState, int]


@pytest.fixture(scope="module")
def simulation_instance_ghz() -> Generator[SimulationInstance, None, None]:
    """Fixture for the GHZ state simulation instance."""
    simulation_state = mqt.debugger.create_ddsim_simulation_state()
    with Path("test/python/resources/bindings/ghz-incorrect.qasm").open(
        encoding=locale.getpreferredencoding(False)
    ) as f:
        code = f.read()
        simulation_state.load_code(code)
    yield (simulation_state, 0)
    mqt.debugger.destroy_ddsim_simulation_state(simulation_state)


@pytest.fixture(scope="module")
def simulation_instance_jumps() -> Generator[SimulationInstance, None, None]:
    """Fixture for the Jumps simulation instance."""
    simulation_state = mqt.debugger.create_ddsim_simulation_state()
    with Path("test/python/resources/bindings/jumps.qasm").open(encoding=locale.getpreferredencoding(False)) as f:
        code = f.read()
        simulation_state.load_code(code)
    yield (simulation_state, 1)
    mqt.debugger.destroy_ddsim_simulation_state(simulation_state)


@pytest.fixture(scope="module")
def simulation_instance_classical() -> Generator[SimulationInstance, None, None]:
    """Fixture for the Classical simulation instance."""
    simulation_state = mqt.debugger.create_ddsim_simulation_state()
    with Path("test/python/resources/bindings/classical.qasm").open(encoding=locale.getpreferredencoding(False)) as f:
        code = f.read()
        simulation_state.load_code(code)
    yield (simulation_state, 2)
    mqt.debugger.destroy_ddsim_simulation_state(simulation_state)


@pytest.fixture(autouse=True)
def simulation_state_cleanup(
    simulation_instance_ghz: SimulationInstance,
    simulation_instance_jumps: SimulationInstance,
    simulation_instance_classical: SimulationInstance,
) -> Generator[None, None, None]:
    """Fixture that resets the simulation state after each test."""
    yield
    simulation_instance_ghz[0].reset_simulation()
    simulation_instance_jumps[0].reset_simulation()
    simulation_instance_classical[0].reset_simulation()
    simulation_instance_ghz[0].clear_breakpoints()
    simulation_instance_jumps[0].clear_breakpoints()
    simulation_instance_classical[0].clear_breakpoints()


def load_fixture(request: pytest.FixtureRequest, name: str) -> tuple[mqt.debugger.SimulationState, int]:
    """Loads a fixture with the given name."""
    return cast("tuple[mqt.debugger.SimulationState, int]", request.getfixturevalue(name))


@pytest.mark.parametrize(
    "simulation_instance", ["simulation_instance_ghz", "simulation_instance_jumps", "simulation_instance_classical"]
)
def test_run(simulation_instance: str, request: pytest.FixtureRequest) -> None:
    """Tests the `run_simulation()` method."""
    (simulation_state, state_id) = load_fixture(request, simulation_instance)
    assert simulation_state.can_step_backward() is False
    simulation_state.run_simulation()
    assert simulation_state.is_finished() == (state_id != 0)
    assert simulation_state.can_step_forward() != (state_id != 0)
    assert simulation_state.did_assertion_fail() == (state_id == 0)


@pytest.mark.parametrize(
    "simulation_instance", ["simulation_instance_ghz", "simulation_instance_jumps", "simulation_instance_classical"]
)
def test_current_instruction(simulation_instance: str, request: pytest.FixtureRequest) -> None:
    """Tests the `get_current_instruction()` method."""
    (simulation_state, state_id) = load_fixture(request, simulation_instance)
    assert simulation_state.get_current_instruction() == 0
    simulation_state.step_forward()
    assert simulation_state.get_current_instruction() == 1
    simulation_state.step_forward()
    assert simulation_state.get_current_instruction() == (2 if state_id != 1 else 4)
    simulation_state.step_backward()
    assert simulation_state.get_current_instruction() == 1
    simulation_state.step_over_backward()
    assert simulation_state.get_current_instruction() == 0
    simulation_state.step_over_forward()
    assert simulation_state.get_current_instruction() == 1


def test_step_out(simulation_instance_jumps: SimulationInstance) -> None:
    """Tests the `step_out()` methods."""
    (simulation_state, _state_id) = simulation_instance_jumps
    simulation_state.set_breakpoint(320)
    simulation_state.run_simulation()
    simulation_state.step_forward()
    simulation_state.step_over_forward()
    assert simulation_state.get_current_instruction() == 12
    assert simulation_state.get_stack_depth() == 2
    assert simulation_state.get_stack_trace(2) == [12, 20]
    simulation_state.step_out_backward()
    assert simulation_state.get_current_instruction() == 20
    assert simulation_state.get_stack_depth() == 1
    simulation_state.step_forward()
    simulation_state.step_over_forward()
    assert simulation_state.get_current_instruction() == 12
    assert simulation_state.get_stack_depth() == 2
    simulation_state.step_out_forward()
    assert simulation_state.get_current_instruction() == 21


@pytest.mark.parametrize(
    "simulation_instance", ["simulation_instance_ghz", "simulation_instance_jumps", "simulation_instance_classical"]
)
def test_run_all(simulation_instance: str, request: pytest.FixtureRequest) -> None:
    """Tests the `run_all()` method."""
    (simulation_state, state_id) = load_fixture(request, simulation_instance)
    failures = simulation_state.run_all()
    assert failures == (2 if state_id == 0 else 0)


@pytest.mark.parametrize("simulation_instance", ["simulation_instance_ghz", "simulation_instance_jumps"])
def test_run_backward(simulation_instance: str, request: pytest.FixtureRequest) -> None:
    """Tests the `run_simulation_backward()` method."""
    (simulation_state, _state_id) = load_fixture(request, simulation_instance)
    bp = simulation_state.set_breakpoint(60)
    simulation_state.run_simulation()
    assert simulation_state.get_current_instruction() == bp
    assert simulation_state.was_breakpoint_hit()
    simulation_state.run_simulation()
    simulation_state.run_simulation_backward()
    assert simulation_state.get_current_instruction() == bp
    simulation_state.run_simulation_backward()
    assert simulation_state.get_current_instruction() == 0


@pytest.mark.parametrize(
    "simulation_instance", ["simulation_instance_ghz", "simulation_instance_jumps", "simulation_instance_classical"]
)
def test_instruction_count(simulation_instance: str, request: pytest.FixtureRequest) -> None:
    """Tests the `get_instruction_count()` method."""
    (simulation_state, state_id) = load_fixture(request, simulation_instance)
    true_counts = [8, 23, 12]
    assert simulation_state.get_instruction_count() == true_counts[state_id]


def test_instruction_positions(simulation_instance_jumps: SimulationInstance) -> None:
    """Tests the `get_instruction_position(instruction)` method."""
    (simulation_state, _state_id) = simulation_instance_jumps
    assert simulation_state.get_instruction_position(0) == (0, 9)
    assert simulation_state.get_instruction_position(1) == (12, 47)
    assert simulation_state.get_instruction_position(16) == (241, 254)


@pytest.mark.parametrize(
    "simulation_instance", ["simulation_instance_ghz", "simulation_instance_jumps", "simulation_instance_classical"]
)
def test_get_num_qubits(simulation_instance: str, request: pytest.FixtureRequest) -> None:
    """Tests the `get_num_qubits()` method."""
    (simulation_state, state_id) = load_fixture(request, simulation_instance)
    assert simulation_state.get_num_qubits() == (3 if state_id != 2 else 4)


def test_access_state(simulation_instance_jumps: SimulationInstance) -> None:
    """Tests the quantum-state-access methods."""
    (simulation_state, _state_id) = simulation_instance_jumps
    simulation_state.run_simulation()

    c = simulation_state.get_amplitude_bitstring("111")
    assert abs(c.imaginary) < 1e-6
    assert abs(c.real - 1 / (2**0.5)) < 1e-6
    c = simulation_state.get_amplitude_bitstring("101")
    assert abs(c.imaginary) < 1e-6
    assert abs(c.real) < 1e-6

    c = simulation_state.get_amplitude_index(0)
    assert abs(c.imaginary) < 1e-6
    assert abs(c.real - 1 / (2**0.5)) < 1e-6
    c = simulation_state.get_amplitude_index(7)
    assert abs(c.imaginary) < 1e-6
    assert abs(c.real - 1 / (2**0.5)) < 1e-6
    c = simulation_state.get_amplitude_index(3)
    assert abs(c.imaginary) < 1e-6
    assert abs(c.real) < 1e-6

    sv = simulation_state.get_state_vector_full()
    assert sv.num_qubits == 3
    assert sv.num_states == 8
    c = sv.amplitudes[7]
    assert abs(c.imaginary) < 1e-6
    assert abs(c.real - 1 / (2**0.5)) < 1e-6
    c = sv.amplitudes[2]
    assert abs(c.imaginary) < 1e-6
    assert abs(c.real) < 1e-6


def test_change_amplitude_value(simulation_instance_ghz: SimulationInstance) -> None:
    """Tests manipulating amplitudes through the bindings."""
    (simulation_state, _state_id) = simulation_instance_ghz
    simulation_state.run_simulation()

    desired = mqt.debugger.Complex(0.25, 0.0)
    simulation_state.change_amplitude_value("111", desired)
    updated = simulation_state.get_amplitude_bitstring("111")
    assert updated.real == pytest.approx(0.25, abs=1e-9)
    assert updated.imaginary == pytest.approx(0.0, abs=1e-9)

    zero_state = simulation_state.get_amplitude_bitstring("000")
    expected_magnitude = math.sqrt(1 - 0.25**2)
    assert zero_state.real == pytest.approx(expected_magnitude, abs=1e-9)
    assert zero_state.imaginary == pytest.approx(0.0, abs=1e-9)


def test_get_state_vector_sub(simulation_instance_classical: SimulationInstance) -> None:
    """Tests the `get_state_vector_sub()` method."""
    (simulation_state, _state_id) = simulation_instance_classical
    simulation_state.set_breakpoint(170)
    simulation_state.run_simulation()
    assert simulation_state.get_current_instruction() == 10
    sv = simulation_state.get_state_vector_sub([0, 1])
    assert sv.amplitudes[0].real == 1 or sv.amplitudes[-1].real == 1


def test_classical_get(simulation_instance_classical: SimulationInstance) -> None:
    """Tests the classical-state-access methods."""
    (simulation_state, _state_id) = simulation_instance_classical
    simulation_state.run_all()
    assert simulation_state.get_num_classical_variables() == 3

    assert simulation_state.get_classical_variable_name(0) == "c[0]"
    assert simulation_state.get_classical_variable_name(1) == "c[1]"
    assert simulation_state.get_classical_variable_name(2) == "c[2]"

    assert simulation_state.get_classical_variable("c[0]").name == "c[0]"
    assert simulation_state.get_classical_variable("c[1]").name == "c[1]"
    assert simulation_state.get_classical_variable("c[2]").name == "c[2]"

    assert simulation_state.get_classical_variable("c[0]").type == mqt.debugger.VariableType.VarBool
    assert simulation_state.get_classical_variable("c[1]").type == mqt.debugger.VariableType.VarBool
    assert simulation_state.get_classical_variable("c[2]").type == mqt.debugger.VariableType.VarBool

    first = simulation_state.get_classical_variable("c[0]").value.bool_value
    assert simulation_state.get_classical_variable("c[1]").value.bool_value == first
    assert simulation_state.get_classical_variable("c[2]").value.bool_value == first
