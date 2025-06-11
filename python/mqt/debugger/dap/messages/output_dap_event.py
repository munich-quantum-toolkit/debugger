# Copyright (c) 2024 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Represents the 'output' DAP event."""

from __future__ import annotations

from typing import Any

from .dap_event import DAPEvent


class OutputDAPEvent(DAPEvent):
    """Represents the 'output' DAP event."""

    event_name = "output"

    category: str
    output: str | None
    group: str | None
    line: int
    column: int
    source: dict[str, Any]

    def __init__(
        self, category: str, output: str | None, group: str | None, line: int, column: int, source: dict[str, Any]
    ) -> None:
        """Create a new 'output' DAP event message.

        Args:
            category (str): The output category.
            output (str): The output text.
            group (str): The output group.
            line (int): The line number.
            column (int): The column number.
            source (dict[str, Any]): The source of the output.
        """
        super().__init__()
        self.category = category
        self.output = output
        self.group = group
        self.line = line
        self.column = column
        self.source = source

    def validate(self) -> None:
        """Validate the 'output' DAP event message after creation."""

    def encode(self) -> dict[str, str]:
        """Encode the 'output' DAP event message as a dictionary.

        Returns:
            dict[str, str]: The encoded 'output' DAP event message.
        """
        d = super().encode()
        d["body"] = {
            "category": self.category,
            "output": self.output,
            "group": self.group,
            "line": self.line,
            "column": self.column,
            "source": self.source,
        }
        return d
