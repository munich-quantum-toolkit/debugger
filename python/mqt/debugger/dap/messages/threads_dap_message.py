# Copyright (c) 2024 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Represents the 'threads' DAP request."""

from __future__ import annotations

from typing import TYPE_CHECKING, Any

from .dap_message import DAPMessage

if TYPE_CHECKING:
    from .. import DAPServer


class ThreadsDAPMessage(DAPMessage):
    """Represents the 'threads' DAP request."""

    message_type_name: str = "threads"

    def __init__(self, message: dict[str, Any]) -> None:
        """Initializes the 'ThreadsDAPMessage' instance.

        Args:
            message (dict[str, Any]): The object representing the 'threads' request.
        """
        super().__init__(message)

    def validate(self) -> None:
        """Validates the 'ThreadsDAPMessage' instance."""

    def handle(self, server: DAPServer) -> dict[str, Any]:
        """Performs the action requested by the 'threads' DAP request.

        Args:
            server (DAPServer): The DAP server that received the request.

        Returns:
            dict[str, Any]: The response to the request.
        """
        d = super().handle(server)
        d["body"] = {"threads": [{"id": 1, "name": "Main Thread"}]}
        return d
