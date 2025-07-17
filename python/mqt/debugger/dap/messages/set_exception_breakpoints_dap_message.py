# Copyright (c) 2024 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Represents the 'setExceptionBreakpoints' DAP request."""

from __future__ import annotations

from typing import TYPE_CHECKING, Any

from .dap_message import DAPMessage

if TYPE_CHECKING:
    from .. import DAPServer


class SetExceptionBreakpointsDAPMessage(DAPMessage):
    """Represents the 'setBreakpoints' DAP request."""

    message_type_name: str = "setExceptionBreakpoints"

    filters: list[str]

    def __init__(self, message: dict[str, Any]) -> None:
        """Initializes the 'SetExceptionBreakpointsDAPMessage' instance.

        Args:
            message (dict[str, Any]): The object representing the 'setExceptionBreakpoints' request.
        """
        self.filters = message["arguments"].get("filters", []) + [
            x["filterId"] for x in message["arguments"].get("filterOptions", [])
        ]
        super().__init__(message)

    def validate(self) -> None:
        """Validates the 'SetExceptionBreakpointsDAPMessage' instance."""

    def handle(self, server: DAPServer) -> dict[str, Any]:
        """Performs the action requested by the 'setExceptionBreakpoints' DAP request.

        Args:
            server (DAPServer): The DAP server that received the request.

        Returns:
            dict[str, Any]: The response to the request.
        """
        server.exception_breakpoints = self.filters
        d = super().handle(server)
        d["body"] = {"breakpoints": [{"verified": True} for x in self.filters]}
        return d
