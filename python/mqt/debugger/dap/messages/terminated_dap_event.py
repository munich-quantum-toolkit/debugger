# Copyright (c) 2024 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Represents the 'terminated' DAP event."""

from __future__ import annotations

from .dap_event import DAPEvent


class TerminatedDAPEvent(DAPEvent):
    """Represents the 'terminated' DAP event."""

    event_name = "terminated"

    def __init__(self) -> None:
        """Initializes the 'TerminatedDAPEvent' instance."""
        super().__init__()

    def validate(self) -> None:
        """Validates the 'TerminatedDAPEvent' instance."""
