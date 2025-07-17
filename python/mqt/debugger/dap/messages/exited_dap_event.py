# Copyright (c) 2024 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Represents the 'exited' DAP event."""

from __future__ import annotations

from .dap_event import DAPEvent


class ExitedDAPEvent(DAPEvent):
    """Represents the 'exited' DAP event."""

    event_name = "exited"

    exit_code: int

    def __init__(self, exit_code: int) -> None:
        """Initializes the 'ExitedDAPEvent' instance.

        Args:
            exit_code (int): The exit code of the process.
        """
        super().__init__()
        self.exit_code = exit_code

    def validate(self) -> None:
        """Validates the 'ExitedDAPEvent' instance."""

    def encode(self) -> dict[str, int]:
        """Encodes the 'ExitedDAPEvent' instance as a dictionary.

        Returns:
            dict[str, int]: The encoded 'ExitedDAPEvent' instance.
        """
        d = super().encode()
        d["body"] = {"exitCode": self.exit_code}
        return d
