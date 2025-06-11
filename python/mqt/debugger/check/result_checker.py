# Copyright (c) 2024 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Handles the checking of the results of the quantum program with assertions."""

from __future__ import annotations

import json
import operator
from dataclasses import dataclass
from pathlib import Path
from typing import TYPE_CHECKING

missing_optionals: list[str] = []
try:
    from scipy.stats import chi2  # type: ignore[import-untyped]
except ImportError:
    missing_optionals.append("scipy")

if TYPE_CHECKING:
    from collections.abc import Sequence
    from io import TextIOWrapper

    from .calibration import Calibration

COLOR_GREEN = "\033[92m"
COLOR_RED = "\033[91m"
COLOR_RESET = "\033[0m"


@dataclass
class Result:
    """Represents the results of a quantum program execution over several shots."""

    num_samples: int
    distribution: dict[tuple[str, ...], list[int]]

    @classmethod
    def load(cls, path: Path | TextIOWrapper, distributions: list[tuple[str, ...]]) -> Result:
        """Load the results from a file.

        Args:
            path (Path | TextIOWrapper): The path to or the file containing the results.
            distributions (list[tuple[str, ...]]): The set of distributions that should be grouped.

        Returns:
            Result: The result of the quantum program.
        """
        if isinstance(path, Path):
            with path.open("r") as f:
                data = json.load(f)
        else:
            data = json.load(path)
        filled_distribution = {key: [0 for _ in range(2 ** len(key))] for key in distributions}
        for entry in data:
            indices = dict.fromkeys(distributions, 0)
            for key in entry:
                val = int(entry[key])
                if val == 0:
                    continue
                parent_distribution_list = [x for x in distributions if key in x]
                if not parent_distribution_list:
                    continue
                parent_distribution = parent_distribution_list[0]
                index = parent_distribution.index(key)
                indices[parent_distribution] += 2**index
            for key, value in indices.items():
                filled_distribution[key][value] += 1
        return Result(len(data), filled_distribution)


def distribution_equal_under_noise(
    distribution: list[int],
    expected_probs: list[float],
    num_samples: int,
    expected_success_probability: float,
    p_value: float = 0.05,
    scale: bool = True,
) -> bool:
    """Check if the distribution is equal to the expected distribution under noise.

    Args:
        distribution (list[int]): The observed distribution.
        expected_probs (list[float]): The expected probabilities.
        num_samples (int): The number of samples.
        expected_success_probability (float): The expected success probability for the program.
        p_value (float): The minimum p-value required to accept the assertion
        scale (bool): If True, scales the distribution to 100 samples.

    Returns:
        bool: True if the distribution is equal to the expected distribution under noise, False otherwise.
    """
    scaled_distribution: Sequence[float] = distribution
    if scale:
        factor = 100 / num_samples
        scaled_distribution = [x * factor for x in distribution]
        num_samples = 100

    expected_with_noise = [
        expected_success_probability * x + (1 - expected_success_probability) / len(expected_probs)
        for x in expected_probs
    ]
    expected_counts = [x * num_samples for x in expected_with_noise]
    expected_counts_no_noise = [x * num_samples for x in expected_probs]
    preprocessed_expected = preprocess_between_characteristic(
        scaled_distribution, expected_counts_no_noise, expected_counts
    )
    result: tuple[float, float] = check_power_divergence(scaled_distribution, preprocessed_expected)
    (statistic, p) = result
    if p == 0.0 and statistic == 0.0:
        return False

    return float(p) > p_value


def filter_out_zeros(observed: Sequence[float], expected: Sequence[float]) -> tuple[list[float], list[float]]:
    """Remove zero values from the observed and expected lists.

    Args:
        observed (Sequence[float]): The observed values.
        expected (Sequence[float]): The expected values.

    Returns:
        tuple[list[float], list[float]]: The filtered observed and expected values.
    """
    observed_filtered = []
    expected_filtered = []
    for o, e in zip(observed, expected):
        if e != 0:
            observed_filtered.append(o)
            expected_filtered.append(e)
        elif o != 0:
            return [], []
    return observed_filtered, expected_filtered


def merge_bins(observed: list[float], expected: list[float], min_size: int = -1) -> tuple[list[float], list[float]]:
    """Merges bins until the expected bins have at least `min_size` samples.

    Args:
        observed (list[float]): The observed values.
        expected (list[float]): The expected values.
        min_size (int, optional): The minimum number of samples in each bin of the expected values. When -1 is selected, select half of the average, but never less than 5. Defaults to -1.

    Returns:
        tuple[list[float], list[float]]: The observed and expected bins, merged according to `min_size`.
    """
    if min_size == -1:
        min_size = int(max(5, max(observed) // len(observed) // 2))
    expected_indexed = list(enumerate(expected))
    expected_indexed.sort(key=operator.itemgetter(1))
    observed = [observed[e[0]] for e in expected_indexed]
    expected = [e[1] for e in expected_indexed]

    while expected[0] < min_size and len(expected) > 2:
        new_expected = expected[0] + expected[1]
        new_observed = observed[0] + observed[1]
        del expected[0]
        del observed[0]
        del expected[0]
        del observed[0]
        for i in range(len(expected)):
            if new_expected < expected[i]:
                expected.insert(i, new_expected)
                observed.insert(i, new_observed)
                break
        else:
            expected.append(new_expected)
            observed.append(new_observed)
    return observed, expected


def preprocess_between_characteristic(
    observed: Sequence[float], expected_no_noise: list[float], expected_noise: list[float]
) -> list[float]:
    """Preprocess the list of expected values according to the 'between' characteristic.

    If O_i is between E_i and Enoise_i, the resulting value is equal to O_i.
    Otherwise, the value between E_i and Enoise_i closer to O_i is selected.

    Args:
        observed (Sequence[float]): The observed values.
        expected_no_noise (list[float]): The expected values without noise.
        expected_noise (list[float]): The expected values with noise.

    Returns:
        list[float]: The preprocessed expected values.
    """
    references = []
    kept = []
    for o, e_1, e_2 in zip(observed, expected_no_noise, expected_noise):
        if (e_2 < e_1 and o > e_2 and o < e_1) or (e_2 > e_1 and (o < e_2 and o > e_1)):
            references.append(o)
            kept.append(False)
        else:
            references.append(e_1 if abs(o - e_1) < abs(o - e_2) else e_2)
            kept.append(True)
    diff = sum(observed) - sum(references)
    if any(kept):
        dt = diff / sum(kept)
        for i in range(len(references)):
            if kept[i]:
                references[i] += dt
    return references


def check_power_divergence(
    observed: Sequence[float], expected: list[float], power: float = 2 / 3
) -> tuple[float, float]:
    """Checks the power divergence statistic for the observed and expected distributions.

    Args:
        observed (Sequence[float]): The observed values.
        expected (list[float]): The expected values.
        power (float, optional): The desired power value. Defaults to 2/3.

    Returns:
        tuple[float, float]: The power divergence statistic and the degree of freedom.
    """
    if missing_optionals:
        raise ImportError(
            "The following optional dependencies are required to use this feature: " + ", ".join(missing_optionals)
        )

    observed, expected = filter_out_zeros(observed, expected)
    if not observed or not expected:
        return 0.0, 0.0
    observed, expected = merge_bins(observed, expected)

    val = 0.0
    for o, e in zip(observed, expected):
        val += o * (((o / e) ** power) - 1)
    val *= 2 / (power * (power + 1))

    return val, chi2.sf(val, len(observed) - 1)


def check_assertion_superposition(
    distribution: list[int], num_samples: int, expected_success_probability: float
) -> bool:
    """Check a superposition assertion based on the distribution.

    Args:
        distribution (list[int]): The observed distribution to check.
        num_samples (int): The number of samples.
        expected_success_probability (float): The expected success probability for the program.

    Returns:
        bool: True if the assertion is satisfied, False otherwise.
    """
    highest = max(distribution) / num_samples
    return (highest - (1 - expected_success_probability) / len(distribution)) < expected_success_probability


def check_assertion_equality(
    assertion: str,
    distribution: list[int],
    num_samples: int,
    expected_success_probability: float,
    p_value: float = 0.05,
) -> bool:
    """Check an equality assertion based on the distribution.

    Args:
        assertion (str): The assertion to check.
        distribution (list[int]): The observed distribution to check.
        num_samples (int): The number of samples.
        expected_success_probability (float): The expected success probability for the program.
        p_value (float): The p-value required to accept the assertion.

    Returns:
        bool: True if the assertion is satisfied, False otherwise.
    """
    expected_distribution = [float(x) for x in assertion[1:].split("}", maxsplit=1)[0].split(",")]
    magnitude = sum(expected_distribution)
    expected_distribution = [x / magnitude for x in expected_distribution]
    return distribution_equal_under_noise(
        distribution, expected_distribution, num_samples, expected_success_probability, p_value
    )


def check_assertion_zero(distribution: list[int], num_samples: int, expected_success_probability: float) -> bool:
    """Check a zero assertion based on the distribution.

    Args:
        distribution (list[int]): The observed distribution to check.
        num_samples (int): The number of samples.
        expected_success_probability (float): The expected success probability for the program.

    Returns:
        bool: True if the assertion is satisfied, False otherwise.
    """
    expected_statevector = [0.0 for _ in distribution]
    expected_statevector[0] = 1.0
    magnitude = sum(abs(x) ** 2 for x in expected_statevector) ** 0.5
    expected_statevector = [x / magnitude for x in expected_statevector]

    expected_distribution: list[float] = [(x * x.conjugate()).real for x in expected_statevector]

    return distribution_equal_under_noise(
        distribution, expected_distribution, num_samples, expected_success_probability
    )


def check_assertion(
    assertion: str,
    distribution: list[int],
    num_samples: int,
    expected_success_probability: float,
    p_value: float = 0.05,
) -> bool:
    """Check an assertion based on the distribution.

    Args:
        assertion (str): The assertion to check.
        distribution (list[int]): The observed distribution to check.
        num_samples (int): The number of samples.
        expected_success_probability (float): The expected success probability for the program.
        p_value (float): The p-value required to accept the assertion.

    Returns:
        bool: True if the assertion is satisfied, False otherwise.
    """
    if assertion.startswith("{superposition}"):
        return check_assertion_superposition(distribution, num_samples, expected_success_probability)
    if assertion.startswith("{zero}"):
        return check_assertion_zero(distribution, num_samples, expected_success_probability)
    return check_assertion_equality(assertion, distribution, num_samples, expected_success_probability, p_value)


def check_result(
    compiled_code: str,
    result_path: Path | TextIOWrapper,
    calibration: Calibration,
    silent: bool = False,
    p_value: float = 0.05,
) -> bool:
    """Check the result of a quantum program against assertions.

    Args:
        compiled_code (str): The compiled code of the quantum program.
        result_path (Path | TextIOWrapper): The path to or the file containing the results.
        calibration (Calibration): The calibration data for the device.
        silent (bool, optional): If true, the output is suppressed. Defaults to False.
        p_value (float, optional): The p-value required to accept the assertion. Defaults to 0.05.

    Returns:
        bool: True if all assertions are satisfied, False otherwise.
    """
    lines = compiled_code.splitlines()
    distributions: list[tuple[str, ...]] = []
    assertions: dict[tuple[str, ...], str] = {}
    for line in lines:
        if not line.startswith("// ASSERT:"):
            break
        var_list = tuple(line.split("(")[1].split(")")[0].split(","))
        distributions.append(var_list)
        assertions[var_list] = f"{{{line.split('{')[1]}"
    result = Result.load(result_path, distributions)

    expected_success_probability = calibration.get_expected_success_probability(compiled_code)

    ok = True
    for key, value in assertions.items():
        res = check_assertion(
            value, result.distribution[key], result.num_samples, expected_success_probability, p_value=p_value
        )
        if not silent:
            if not res:
                print(f"{COLOR_RED}Assertion {key} failed.{COLOR_RESET}")  # noqa: T201
            else:
                print(f"{COLOR_GREEN}Assertion {key} passed.{COLOR_RESET}")  # noqa: T201
        ok &= res
    return ok
