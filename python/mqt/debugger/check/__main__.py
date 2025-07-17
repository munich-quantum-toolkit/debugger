# Copyright (c) 2024 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Allows to run the runtime check as a module."""

from __future__ import annotations

from . import runtime_check

if __name__ == "__main__":
    runtime_check.main()
