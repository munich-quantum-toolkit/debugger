# Copyright (c) 2024 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Represents the 'stopped' DAP event."""

from __future__ import annotations

import enum

from .dap_event import DAPEvent


class StopReason(enum.Enum):
    """Represents the reason for stopping."""

    STEP = "step"
    BREAKPOINT = "breakpoint"
    EXCEPTION = "exception"
    PAUSE = "pause"
    ENTRY = "entry"
    GOTO = "goto"
    BREAKPOINT_FUNCTION = "function breakpoint"
    BREAKPOINT_DATA = "data breakpoint"
    BREAKPOINT_INSTRUCTION = "instruction breakpoint"


class StoppedDAPEvent(DAPEvent):
    """Represents the 'stopped' DAP event."""

    event_name = "stopped"

    reason: StopReason
    description: str

    def __init__(self, reason: StopReason, description: str) -> None:
        """Create a new 'stopped' DAP event message.

        Args:
            reason (StopReason): The reason for stopping.
            description (str): The description of the stop.
        """
        super().__init__()
        self.reason = reason
        self.description = description

    def validate(self) -> None:
        """Validate the 'stopped' DAP event message after creation."""

    def encode(self) -> dict[str, str]:
        """Encode the 'stopped' DAP event message as a dictionary.

        Returns:
            dict[str, str]: The encoded 'stopped' DAP event message.
        """
        d = super().encode()
        d["body"] = {}
        d["body"]["reason"] = self.reason.value
        d["body"]["description"] = self.description
        d["body"]["threadId"] = 1
        d["body"]["text"] = self.description
        d["body"]["allThreadsStopped"] = True
        return d
