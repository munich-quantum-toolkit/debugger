# Copyright (c) 2024 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Represents a DAP event message."""

from __future__ import annotations

from abc import ABC, abstractmethod
from typing import Any


class DAPEvent(ABC):
    """Represents a generic DAP event message."""

    event_name: str = "None"

    def __init__(self) -> None:
        """Create a new DAP event message."""
        super().__init__()
        self.validate()

    @abstractmethod
    def validate(self) -> None:
        """Validate the DAP event message after creation.

        Raises an exception if the message is invalid.
        """
        ...

    def encode(self) -> dict[str, Any]:
        """Encode the DAP event message as a dictionary.

        Returns:
            dict[str, Any]: The encoded DAP event message.
        """
        return {"type": "event", "event": self.event_name}
