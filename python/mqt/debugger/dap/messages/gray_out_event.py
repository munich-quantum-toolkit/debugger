# Copyright (c) 2024 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Represents the 'grayOut' DAP custom event."""

from __future__ import annotations

from typing import Any

from .dap_event import DAPEvent


class GrayOutDAPEvent(DAPEvent):
    """Represents the 'grayOut' DAP event."""

    event_name = "grayOut"

    ranges: list[tuple[int, int]]
    source: dict[str, Any]

    def __init__(self, ranges: list[tuple[int, int]], source: dict[str, Any]) -> None:
        """Create a new 'grayOut' DAP event message.

        Args:
            ranges (list[tuple[int, int]]): The ranges to gray out.
            source (dict[str, Any]): The source of the ranges.
        """
        super().__init__()
        self.ranges = ranges
        self.source = source

    def validate(self) -> None:
        """Validate the 'grayOut' DAP event message after creation."""

    def encode(self) -> dict[str, str]:
        """Encode the 'grayOut' DAP event message as a dictionary.

        Returns:
            dict[str, str]: The encoded 'grayOut' DAP event message.
        """
        d = super().encode()
        d["body"] = {
            "ranges": self.ranges,
            "source": self.source,
        }
        return d
