# Copyright (c) 2024 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Tests the compilation procedures for assertion programs."""

from __future__ import annotations

import json
import locale
import os
import random
import re
import shutil
import sys
import tempfile
from pathlib import Path
from typing import TYPE_CHECKING, Any

import pytest

from mqt.debugger import check
from mqt.debugger.check import result_checker, runtime_check

if TYPE_CHECKING:
    import types
    from io import TextIOWrapper

CALIBRATION = check.Calibration(0.1, 0.1, 0.1)
BASE_PATH = Path("test/python/resources/compilation/")


class GeneratedOutput:
    """A context manager for generating output files with a given distribution."""

    file: TextIOWrapper

    variables: list[str]
    expected_distribution: list[float]
    num_samples: int
    success_probability: float

    def generate_output(self) -> list[dict[str, Any]]:
        """Generate a list of sample output dictionaries.

        Returns:
            list[dict[str, any]]: The sample output dictionaries.
        """
        samples: list[dict[str, Any]] = []

        for _ in range(self.num_samples):
            r = random.random()  # noqa: S311
            found_index = -1
            for i, likelihood in enumerate(self.expected_distribution):
                if r < likelihood:
                    found_index = i
                    break
                r -= likelihood

            if random.random() > self.success_probability:  # noqa: S311
                found_index = random.randrange(0, len(self.expected_distribution))  # noqa: S311

            result = {}
            for v in self.variables:
                result[v] = found_index & 1
                found_index >>= 1
            samples.append(result)
        return samples

    def __init__(
        self, variables: list[str], expected_distribution: list[float], num_samples: int, success_probability: float
    ) -> None:
        """Create a new GeneratedOutput context manager.

        Args:
            variables (list[str]): The variables in the output.
            expected_distribution (list[float]): The expected distribution of the output.
            num_samples (int): The number of samples to generate.
            success_probability (float): The success probability of the output.
        """
        self.variables = variables
        self.expected_distribution = expected_distribution
        self.num_samples = num_samples
        self.success_probability = success_probability

    def __enter__(self) -> tuple[str, TextIOWrapper]:
        """The context manager entry point.

        Returns:
            TextIOWrapper: The file containing the generated output.
        """
        with tempfile.NamedTemporaryFile(mode="w", delete=False, encoding=locale.getpreferredencoding(False)) as f:
            name = f.name
            generated = self.generate_output()
            json.dump(generated, f)

        self.file = Path(name).open("r", encoding=locale.getpreferredencoding(False))
        return name, self.file

    def __exit__(
        self,
        _exc_type: type[BaseException] | None,
        _exc_value: BaseException | None,
        _traceback: types.TracebackType | None,
    ) -> None:
        """The context manager exit point.

        Args:
            _exc_type (_type_): The type of the exception.
            _exc_value (_type_): The exception value.
            _traceback (_type_): The traceback.
        """
        name = self.file.name
        self.file.close()
        Path(name).unlink()


@pytest.fixture(scope="module")
def compiled_slice_1() -> str:
    """A fixture for slice 1 of the compiled program.

    Returns:
        str: The compiled program slice code.
    """
    with (BASE_PATH / "test_program_compiled" / "slice_1.qasm").open("r") as f:
        return f.read()


def test_correct_good_sample_size(compiled_slice_1: str) -> None:
    """Test the correctness of a run result with a large enough sample size.

    Args:
        compiled_slice_1 (str): The compiled program slice code.
    """
    random.seed(12345)
    results = []
    for _ in range(100):
        expected_success_probability = CALIBRATION.get_expected_success_probability(compiled_slice_1)
        with GeneratedOutput(["test_q0", "test_q1"], [0.5, 0, 0, 0.5], 250, expected_success_probability) as (_, o):
            result = check.check_result(compiled_slice_1, o, CALIBRATION, silent=True)
            results.append(result)
    errors = 100 - sum(results)
    assert errors <= 5


def test_correct_bad_sample_size(compiled_slice_1: str) -> None:
    """Test the correctness of a run result with a bad sample size.

    Args:
        compiled_slice_1 (str): The compiled program slice code.
    """
    random.seed(12345)
    results = []
    for _ in range(100):
        expected_success_probability = CALIBRATION.get_expected_success_probability(compiled_slice_1)
        with GeneratedOutput(["test_q0", "test_q1"], [0.5, 0, 0, 0.5], 25, expected_success_probability) as (_, o):
            result = check.check_result(compiled_slice_1, o, CALIBRATION, silent=True)
            results.append(result)
    errors = 100 - sum(results)
    assert errors >= 5


def test_incorrect_bad_sample_size(compiled_slice_1: str) -> None:
    """Test the incorrectness of a run result with a bad sample size.

    Args:
        compiled_slice_1 (str): The compiled program slice code.
    """
    random.seed(12345)
    results = []
    for _ in range(100):
        expected_success_probability = CALIBRATION.get_expected_success_probability(compiled_slice_1)
        with GeneratedOutput(["test_q0", "test_q1"], [0.75, 0, 0, 0.25], 25, expected_success_probability) as (_, o):
            result = check.check_result(compiled_slice_1, o, CALIBRATION, silent=True)
            results.append(result)
    errors = 100 - sum(results)
    # Due to the low sample size, the checker does not find at least one out of 4 errors
    assert errors <= 75


def test_incorrect_good_sample_size(compiled_slice_1: str) -> None:
    """Test the incorrectness of a run result with a good sample size.

    Args:
        compiled_slice_1 (str): The compiled program slice code.
    """
    random.seed(12345)
    results = []
    for _ in range(100):
        expected_success_probability = CALIBRATION.get_expected_success_probability(compiled_slice_1)
        with GeneratedOutput(["test_q0", "test_q1"], [0.75, 0, 0, 0.25], 250, expected_success_probability) as (_, o):
            result = check.check_result(compiled_slice_1, o, CALIBRATION, silent=True)
            results.append(result)
    errors = 100 - sum(results)
    assert errors >= 75


def test_sample_estimate(compiled_slice_1: str) -> None:
    """Test the estimation of required shots.

    Args:
        compiled_slice_1 (str): The compiled program slice code.
    """
    random.seed(12345)
    n = check.estimate_required_shots(compiled_slice_1, CALIBRATION)
    assert n == 100


def check_dir_contents_and_delete(directory: Path, expected: dict[str, str]) -> None:
    """Check the contents of a directory against expected values.

    Args:
        directory (Path): The directory to check.
        expected (dict[Path, str]): A dictionary mapping file paths to their expected contents.
    """
    contents: dict[str, str] = {}
    for dir_path, _, files in os.walk(str(directory)):
        for file in files:
            path = Path(dir_path) / file
            with path.open("r") as f:
                contents[str(path.relative_to(directory))] = f.read()

    shutil.rmtree(directory)
    assert len(contents) == len(expected), f"Expected {len(expected)} files, but found {len(contents)}."
    for name, expected_contents in expected.items():
        assert name in contents, f"File {name} not found in directory."
        assert contents[name] == expected_contents, f"Contents of {name} do not match expected value."


def test_main_prepare(compiled_slice_1: str, monkeypatch: pytest.MonkeyPatch) -> None:
    """Test the correctness of the "prepare" mode of the main function.

    Args:
        compiled_slice_1 (str): The compiled program slice code.
        monkeypatch (pytest.MonkeyPatch): Monkeypatch fixture for testing.
    """
    Path("tmp-test").mkdir(exist_ok=True)
    monkeypatch.setattr(
        sys,
        "argv",
        [
            "runtime_check.py",
            "prepare",
            str(BASE_PATH.joinpath("original.qasm")),
            "-o",
            "tmp-test",
        ],
    )
    runtime_check.main()
    check_dir_contents_and_delete(Path("tmp-test"), {"slice_1.qasm": compiled_slice_1})


def test_main_check_equality_sv(monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]) -> None:
    """Test the correctness of the "check" mode of the main function with a statevector-equality assertion.

    Args:
        monkeypatch (pytest.MonkeyPatch): Monkeypatch fixture for testing.
        capsys (pytest.CaptureFixture): Capture fixture for testing.
    """
    random.seed(12345)
    with GeneratedOutput(["test_q0", "test_q1"], [0.5, 0, 0, 0.5], 250, 0.9) as (path, _):
        monkeypatch.setattr(
            sys,
            "argv",
            [
                "runtime_check.py",
                "--calibration",
                str(BASE_PATH.joinpath("calibration.json")),
                "check",
                str(path),
                "--dir",
                str(BASE_PATH.joinpath("test_program_compiled")),
                "--slice",
                "1",
                "-p",
                "0.05",
            ],
        )
        runtime_check.main()
    captured = capsys.readouterr()
    assert "passed" in captured.out


def test_main_check_equality_circuit(monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]) -> None:
    """Test the correctness of the "check" mode of the main function with a circuit-equality assertion.

    Args:
        monkeypatch (pytest.MonkeyPatch): Monkeypatch fixture for testing.
        capsys (pytest.CaptureFixture): Capture fixture for testing.
    """
    random.seed(12345)
    with GeneratedOutput(["test_q0", "test_q1"], [1, 0, 0, 0], 250, 0.99) as (path, _):
        monkeypatch.setattr(
            sys,
            "argv",
            [
                "runtime_check.py",
                "--calibration",
                str(BASE_PATH.joinpath("calibration.json")),
                "check",
                str(path),
                "--dir",
                str(BASE_PATH.joinpath("test_program_compiled")),
                "--slice",
                "3",
                "-p",
                "0.05",
            ],
        )
        runtime_check.main()
    captured = capsys.readouterr()
    assert "passed" in captured.out


def test_main_check_sup(monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]) -> None:
    """Test the correctness of the "check" mode of the main function with a superposition assertion.

    Args:
        monkeypatch (pytest.MonkeyPatch): Monkeypatch fixture for testing.
        capsys (pytest.CaptureFixture): Capture fixture for testing.
    """
    random.seed(12345)
    with GeneratedOutput(["test_q0", "test_q1"], [0.5, 0, 0, 0.5], 250, 0.9) as (path, _):
        monkeypatch.setattr(
            sys,
            "argv",
            [
                "runtime_check.py",
                "--calibration",
                str(BASE_PATH.joinpath("calibration.json")),
                "check",
                str(path),
                "--dir",
                str(BASE_PATH.joinpath("test_program_compiled")),
                "--slice",
                "2",
                "-p",
                "0.05",
            ],
        )
        runtime_check.main()
    captured = capsys.readouterr()
    assert "passed" in captured.out


def test_main_check_eq_incorrect(monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]) -> None:
    """Test the correctness of the "check" mode of the main function with a statevector-equality assertion with incorrect results.

    Args:
        monkeypatch (pytest.MonkeyPatch): Monkeypatch fixture for testing.
        capsys (pytest.CaptureFixture): Capture fixture for testing.
    """
    random.seed(12345)
    with GeneratedOutput(["test_q0", "test_q1"], [0.9, 0, 0, 0.1], 250, 0.9) as (path, _):
        monkeypatch.setattr(
            sys,
            "argv",
            [
                "runtime_check.py",
                "--calibration",
                str(BASE_PATH.joinpath("calibration.json")),
                "check",
                str(path),
                "--dir",
                str(BASE_PATH.joinpath("test_program_compiled")),
                "--slice",
                "1",
                "-p",
                "0.05",
            ],
        )
        runtime_check.main()
    captured = capsys.readouterr()
    assert "passed" not in captured.out


def test_main_check_sup_incorrect(monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]) -> None:
    """Test the correctness of the "check" mode of the main function with a superposition assertion with incorrect results.

    Args:
        monkeypatch (pytest.MonkeyPatch): Monkeypatch fixture for testing.
        capsys (pytest.CaptureFixture): Capture fixture for testing.
    """
    random.seed(12345)
    with GeneratedOutput(["test_q0", "test_q1"], [1, 0, 0, 0], 250, 1.0) as (path, _):
        monkeypatch.setattr(
            sys,
            "argv",
            [
                "runtime_check.py",
                "--calibration",
                str(BASE_PATH.joinpath("calibration.json")),
                "check",
                str(path),
                "--dir",
                str(BASE_PATH.joinpath("test_program_compiled")),
                "--slice",
                "2",
                "-p",
                "0.05",
            ],
        )
        runtime_check.main()
    captured = capsys.readouterr()
    assert "passed" not in captured.out


def test_main_shots(monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]) -> None:
    """Test the correctness of the "check" mode of the main function.

    Args:
        monkeypatch (pytest.MonkeyPatch): Monkeypatch fixture for testing.
        capsys (pytest.CaptureFixture): Capture fixture for testing.
    """
    random.seed(12345)
    monkeypatch.setattr(
        sys,
        "argv",
        [
            "runtime_check.py",
            "--calibration",
            str(BASE_PATH.joinpath("calibration.json")),
            "shots",
            str(BASE_PATH.joinpath("test-shots.qasm")),
            "-p",
            "0.05",
            "--trials",
            "100",
            "--accuracy",
            "0.9",
        ],
    )
    runtime_check.main()
    out = capsys.readouterr().out
    match = re.match(r"^Estimated required shots: (\d+)$", out)
    assert match is not None, f"Output did not match expected format: {out}"
    shots = int(match.group(1))
    assert shots == 180, f"Expected 100 shots, but got {shots}."


def test_contribution_equal_under_noise_big_difference() -> None:
    """Test the correctness of the `distribution_equal_under_noise` function when distributions are very different."""
    assert not result_checker.distribution_equal_under_noise(
        [100, 0, 0, 100], [0, 0.5, 0.5, 0], 200, 1.0, 0.05, scale=False
    )


def test_check_power_divergence_zeros() -> None:
    """Tests that `check_power_divergence` correctly returns `False` if the expected distribution has a zero entry while the observed distribution's entry is non-zero."""
    statistic, p = result_checker.check_power_divergence([99, 1, 0, 0], [100, 0, 0, 0])
    assert statistic == 0.0
    assert p == 0.0
