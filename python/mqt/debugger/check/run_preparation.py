# Copyright (c) 2024 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Module responsible for the preparation of running programs on real hardware."""

from __future__ import annotations

import random
from pathlib import Path
from typing import TYPE_CHECKING

import mqt.debugger as dbg

from . import result_checker

if TYPE_CHECKING:
    from .calibration import Calibration


def extract_assertions_from_code(code: str) -> list[str]:
    """Extract the assertions from the given code.

    Args:
        code (str): The code to extract the assertions from.

    Returns:
        list[str]: The extracted assertions.
    """
    return [x[11:] for x in code.splitlines() if x.startswith("// ASSERT: ")]


def start_compilation(code: Path, output_dir: Path) -> None:
    """Start the compilation of the assertion program.

    Args:
        code (Path): The path to the assertion program in extended OpenQASM format.
        output_dir (Path): The directory to store the compiled slices.
    """
    state = dbg.create_ddsim_simulation_state()
    with code.open("r") as f:
        code_str = f.read()
    state.load_code(code_str)
    i = 0
    while True:
        i += 1
        settings = dbg.CompilationSettings(
            opt=0,
            slice_index=i - 1,
        )
        compiled = state.compile(settings)
        if not compiled:
            break
        with (output_dir / f"slice_{i}.qasm").open("w") as f:
            f.write(compiled)
    dbg.destroy_ddsim_simulation_state(state)


# -------------------------


def sample_distribution(expected_distribution_under_noise: list[float], num_samples: int) -> list[int]:
    """Sample from a given distribution.

    Args:
        expected_distribution_under_noise (list[float]): The expected distribution under noise.
        num_samples (int): The number of samples to take.

    Returns:
        list[int]: The sampled distribution.
    """
    samples: list[int] = [0 for _ in expected_distribution_under_noise]
    for _ in range(num_samples):
        rnd = random.random()  # noqa: S311
        for i, likelihood in enumerate(expected_distribution_under_noise):
            if rnd < likelihood:
                samples[i] += 1
                break
            rnd -= likelihood
    return samples


def estimate_required_shots_for_assertion(
    assertion: str,
    expected_success_probability: float,
    p_value: float = 0.05,
    num_trials: int = 1000,
    accuracy: float = 0.95,
) -> int:
    """Estimate the required shots for a given assertion.

    Args:
        assertion (str): The assertion to estimate the required shots for.
        expected_success_probability (float): The expected success probability.
        p_value (float, optional): The p-value required to accept the assertion. Defaults to 0.05.
        num_trials (int, optional): The number of trials that should be checked. Defaults to 1000.
        accuracy (float, optional): The desired accuracy to decide on a number of shots. Defaults to 0.95.

    Returns:
        int: The estimated number of shots required for the assertion.
    """
    expected_distribution: list[float] = []
    num_variables = assertion.split("(")[1].split(")")[0].count(",") + 1
    if "{zero}" in assertion:
        expected_distribution = [0.0] * 2**num_variables
        expected_distribution[0] = 1.0
    elif "{superposition}" in assertion:
        # superposition assertions should detect any superposition, but here we just consider the uniform one.
        expected_distribution = [1.0 / 2**num_variables] * 2**num_variables
    else:
        expected_distribution = [float(x) for x in assertion.split("{")[1].split("}")[0].split(",")]
        norm = sum(expected_distribution)
        expected_distribution = [x / norm for x in expected_distribution]

    expected_with_noise = [
        expected_success_probability * x + (1 - expected_success_probability) / len(expected_distribution)
        for x in expected_distribution
    ]
    num_samples = len(expected_distribution) * 5
    assertion_string = assertion[assertion.index("{") :]

    all_samples = [[0 for _ in expected_distribution] for _ in range(num_trials)]
    previous_samples = 0

    while True:
        correct = 0
        for i in range(num_trials):
            new_samples = sample_distribution(expected_with_noise, num_samples - previous_samples)
            all_samples[i] = [a + b for a, b in zip(all_samples[i], new_samples)]
            if result_checker.check_assertion(
                assertion_string, all_samples[i], num_samples, expected_success_probability, p_value
            ):
                correct += 1
        reported_accuracy = correct / num_trials
        previous_samples = num_samples
        if reported_accuracy >= accuracy:
            return num_samples
        num_samples += len(expected_distribution) * 5


def estimate_required_shots(
    program: str, calibration: Calibration, p_value: float = 0.05, num_trials: int = 1000, accuracy: float = 0.95
) -> int:
    """Estimate the required shots for a given program.

    Args:
        program (str): The program to estimate the required shots for.
        calibration (Calibration): The calibration data for the device.
        p_value (float, optional): The p-value required to accept an assertion. Defaults to 0.05.
        num_trials (int, optional): The number of trials that should be checked. Defaults to 1000.
        accuracy (float, optional): The desired accuracy to select a number of shots. Defaults to 0.95.

    Returns:
        int: The estimated number of shots required for the program.
    """
    expected_success_probability = calibration.get_expected_success_probability(program)
    assertions = extract_assertions_from_code(program)
    required_shots = [
        estimate_required_shots_for_assertion(a, expected_success_probability, p_value, num_trials, accuracy)
        for a in assertions
    ]
    return max(required_shots)


def estimate_required_shots_from_path(
    compiled_program: str | Path,
    calibration: Calibration,
    p_value: float = 0.05,
    num_trials: int = 1000,
    accuracy: float = 0.95,
) -> int:
    """Estimate the required number of shots for a program given by a path to a program file.

    Args:
        compiled_program (str | Path): The path to the program to check.
        calibration (Calibration): The calibration data for the device
        p_value (float, optional): The p-value required to accept an assertion. Defaults to 0.05.
        num_trials (int, optional): The number of trials to check. Defaults to 1000.
        accuracy (float, optional): The desired accuracy to select a number of shots. Defaults to 0.95.

    Returns:
        int: The estimated number of shots required for the program.
    """
    if isinstance(compiled_program, str):
        compiled_program = Path(compiled_program)
    with compiled_program.open("r") as f:
        program = f.read()
        return estimate_required_shots(program, calibration, p_value, num_trials, accuracy)
