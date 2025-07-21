# Copyright (c) 2024 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""This module handles running assertion programs on real hardware and using statistical tests to check the results."""

from __future__ import annotations

import argparse
import json
import locale
from pathlib import Path

from . import result_checker, run_preparation
from .calibration import Calibration


def main() -> None:
    """The main function."""
    parser = argparse.ArgumentParser(description="Compile assertion programs for real hardware.")
    subparsers = parser.add_subparsers(dest="mode", required=True, help="The mode to run the program in.")
    parser.add_argument(
        "--calibration", type=Path, help="The path to a calibration file containing device information.", default=None
    )

    # Add the subparser for the preparation mode.
    sub_preparation = subparsers.add_parser(
        "prepare", help="Prepare the assertion program for running on real hardware."
    )
    sub_preparation.add_argument(
        "code", type=Path, help="The path to the assertion program in extended OpenQASM format."
    )
    sub_preparation.add_argument(
        "--output-dir", "-o", type=Path, help="The directory to store the compiled slices.", default="."
    )

    # Add the subparser for the checking mode.
    sub_checker = subparsers.add_parser("check", help="Check the results of the assertion program.")
    sub_checker.add_argument("results", type=Path, help="The path to a JSON file containing all results.")
    sub_checker.add_argument("--dir", "-d", type=Path, help="The path to the compiled program.", default=".")
    sub_checker.add_argument("--slice", "-s", type=int, help="The slice index to check.", default=1)
    sub_checker.add_argument("-p", type=float, help="The minimal desired p-value to accept an assertion.", default=0.05)

    # Add the subparser for shot estimation.
    sub_checker = subparsers.add_parser(
        "shots", help="Estimate the number of shots required to evaluate the assertion."
    )
    sub_checker.add_argument("slice", type=Path, help="The path to a compiled assertion program slice.")
    sub_checker.add_argument("-p", type=float, help="The minimal desired p-value to accept an assertion.", default=0.05)
    sub_checker.add_argument(
        "--trials", type=int, help="The number of trials for probabilistic estimation.", default=1000
    )
    sub_checker.add_argument(
        "--accuracy", type=float, help="The desired accuracy to report a sample count.", default=0.95
    )

    args = parser.parse_args()

    if args.calibration is not None:
        with args.calibration.open("r") as f:
            calibration_data = Calibration(**json.load(f))
    else:
        calibration_data = Calibration.example()

    if args.mode == "prepare":
        run_preparation.start_compilation(args.code, args.output_dir)
    elif args.mode == "check":
        with (args.dir / f"slice_{args.slice}.qasm").open("r", encoding=locale.getpreferredencoding(False)) as f:
            compiled_code = f.read()
        result_checker.check_result(compiled_code, args.results, calibration_data, p_value=args.p)
    elif args.mode == "shots":
        result = run_preparation.estimate_required_shots_from_path(
            args.slice, calibration_data, args.p, args.trials, args.accuracy
        )
        print(f"Estimated required shots: {result}")  # noqa: T201
