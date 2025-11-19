# Copyright (c) 2024 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Represents the bit chagenge DAP request."""

from __future__ import annotations

from typing import TYPE_CHECKING, Any

from .dap_message import DAPMessage

if TYPE_CHECKING:
    from .. import DAPServer


class BitChangeDAPMessage(DAPMessage):
    """Represents the 'bitChange' DAP request."""

    message_type_name: str = "bitChange"

    variable_name: str | None

    def __init__(self, message: dict[str, Any]) -> None:
        """Initializes the 'BitChangeDAPMessage' instance.

        Args:
            message (dict[str, Any]): The object representing the 'bitChange' request.
        """
        arguments = message.get("arguments", {})
        self.variable_name = arguments.get("variableName")
        super().__init__(message)

    def validate(self) -> None:
        """Validates the 'BitChangeDAPMessage' instance."""
        if not isinstance(self.variable_name, str) or not self.variable_name:
            msg = "The 'bitChange' request requires a non-empty 'variableName' argument."
            raise ValueError(msg)

    def handle(self, server: DAPServer) -> dict[str, Any]:
        """Performs the action requested by the 'bitChange' DAP request.

        Args:
            server (DAPServer): The DAP server that received the request.

        Returns:
            dict[str, Any]: The response to the request.
        """
        server.simulation_state.change_classical_value(self.variable_name)
        return super().handle(server)
