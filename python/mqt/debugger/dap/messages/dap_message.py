# Copyright (c) 2024 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Represents a DAP request message."""

from __future__ import annotations

from abc import ABC, abstractmethod
from typing import TYPE_CHECKING, Any

if TYPE_CHECKING:
    from .. import DAPServer


class DAPMessage(ABC):
    """Represents a generic DAP request message."""

    message_type_name: str = "None"

    sequence_number: int

    def __init__(self, message: dict[str, Any]) -> None:
        """Creates a new DAPMessage.

        Args:
            message (dict[str, Any]): The object containing the message data.
        """
        super().__init__()
        self.sequence_number = message["seq"]
        self.validate()

    @abstractmethod
    def validate(self) -> None:
        """Validate the DAP request message after creation.

        Raises an exception if the message is invalid.
        """
        ...

    def handle(self, _server: DAPServer) -> dict[str, Any]:
        """Performs the action requested by the DAP request message and returns the response.

        Args:
            server (DAPServer): The DAP server that received the request.

        Returns:
            dict[str, Any]: The response to the request.
        """
        return {
            "type": "response",
            "request_seq": self.sequence_number,
            "success": True,
            "command": self.message_type_name,
        }
