# Copyright (c) 2024 - 2026 Chair for Design Automation, TUM
# Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Tests diagnosis methods of the debugger."""

from __future__ import annotations

import locale
from pathlib import Path
from typing import TYPE_CHECKING

from mqt.debugger import (
    ErrorCauseType,
    create_ddsim_simulation_state,
    destroy_ddsim_simulation_state,
)

if TYPE_CHECKING:
    from mqt.debugger import SimulationState


def load_instance(name: str) -> SimulationState:
    """Loads a quantum simulation and debugging instance from a file.

    Args:
        name (str): The name of the instance to load.

    Returns:
        SimulationState: The generated simulation state.
    """
    state = create_ddsim_simulation_state()
    code = Path(f"test/python/resources/diagnosis/{name}.qasm").read_text(encoding=locale.getpreferredencoding(False))
    state.load_code(code)
    return state


def test_data_dependencies() -> None:
    """Test the data dependency analysis."""
    s = load_instance("dependencies")
    dependencies = s.get_diagnostics().get_data_dependencies(6)
    assert dependencies == [1, 2, 3, 6]
    dependencies = s.get_diagnostics().get_data_dependencies(7)
    assert dependencies == [1, 2, 3, 4, 5, 7]
    destroy_ddsim_simulation_state(s)


def test_data_dependencies_jumps() -> None:
    """Test the data dependency analysis in code with jumps."""
    s = load_instance("dependencies-with-jumps")
    dependencies = s.get_diagnostics().get_data_dependencies(4)
    assert dependencies == [2, 3, 4]
    dependencies = s.get_diagnostics().get_data_dependencies(9)
    assert dependencies == [2, 3, 4, 6, 7, 8, 9]
    destroy_ddsim_simulation_state(s)


def test_control_always_zero() -> None:
    """Test the control-always-zero error diagnosis."""
    s = load_instance("control-always-zero")
    s.run_simulation()
    causes = s.get_diagnostics().potential_error_causes()

    assert len(causes) == 2

    assert causes[0].type_ == ErrorCauseType.ControlAlwaysZero
    assert causes[0].instruction == 3

    assert causes[1].type_ == ErrorCauseType.ControlAlwaysZero
    assert causes[1].instruction == 12


def test_missing_interaction() -> None:
    """Test the missing-interaction error diagnosis."""
    s = load_instance("missing-interaction")
    s.run_simulation()
    causes = [x for x in s.get_diagnostics().potential_error_causes() if x.type_ == ErrorCauseType.MissingInteraction]
    assert len(causes) == 0
    s.run_simulation()
    causes = [x for x in s.get_diagnostics().potential_error_causes() if x.type_ == ErrorCauseType.MissingInteraction]
    assert len(causes) == 1
    assert causes[0].instruction == 4


def test_zero_control_listing() -> None:
    """Test the zero-control list."""
    s = load_instance("control-always-zero")
    s.run_simulation()
    zero_controls = s.get_diagnostics().get_zero_control_instructions()
    assert zero_controls == [3, 12]


def test_data_dependencies_with_callers() -> None:
    """Tests the data dependency analysis with enabling callers."""
    s = load_instance("data-dependencies-with-callers")
    s.run_simulation()
    dependencies = s.get_diagnostics().get_data_dependencies(2, include_callers=True)
    assert dependencies == [2, 4, 6, 8, 9]

    dependencies = s.get_diagnostics().get_data_dependencies(7, include_callers=True)
    assert dependencies == [2, 4, 6, 7]
    # 8 and 9 are not included `test` doesn't have unknown callers in this case, so the analysis won't include all callers.
