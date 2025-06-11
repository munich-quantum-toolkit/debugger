# Copyright (c) 2024 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Represents the 'terminate' DAP request."""

from __future__ import annotations

from typing import TYPE_CHECKING, Any

import mqt.debugger

from .dap_message import DAPMessage

if TYPE_CHECKING:
    from .. import DAPServer


class TerminateDAPMessage(DAPMessage):
    """Represents the 'terminate' DAP request."""

    message_type_name: str = "terminate"

    def __init__(self, message: dict[str, Any]) -> None:
        """Initializes the 'TerminateDAPMessage' instance.

        Args:
            message (dict[str, Any]): The object representing the 'terminate' request.
        """
        super().__init__(message)

    def validate(self) -> None:
        """Validates the 'TerminateDAPMessage' instance."""

    def handle(self, server: DAPServer) -> dict[str, Any]:
        """Performs the action requested by the 'terminate' DAP request.

        Args:
            server (DAPServer): The DAP server that received the request.

        Returns:
            dict[str, Any]: The response to the request.
        """
        mqt.debugger.destroy_ddsim_simulation_state(server.simulation_state)
        return super().handle(server)
