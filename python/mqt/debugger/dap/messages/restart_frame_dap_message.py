# Copyright (c) 2024 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Represents the 'restartFrame' DAP request."""

from __future__ import annotations

from typing import TYPE_CHECKING, Any

from .dap_message import DAPMessage

if TYPE_CHECKING:
    from .. import DAPServer


class RestartFrameDAPMessage(DAPMessage):
    """Represents the 'restartFrame' DAP request."""

    message_type_name: str = "restartFrame"
    frame: int

    def __init__(self, message: dict[str, Any]) -> None:
        """Initializes the 'RestartFrameDAPMessage' instance.

        Args:
            message (dict[str, Any]): The object representing the 'restartFrame' request.
        """
        super().__init__(message)
        self.frame = message["arguments"]["frameId"]

    def validate(self) -> None:
        """Validates the 'RestartFrameDAPMessage' instance."""

    def handle(self, server: DAPServer) -> dict[str, Any]:
        """Performs the action requested by the 'restartFrame' DAP request.

        Args:
            server (DAPServer): The DAP server that received the request.

        Returns:
            dict[str, Any]: The response to the request.
        """
        while server.simulation_state.get_stack_depth() >= self.frame:
            server.simulation_state.step_out_backward()
        server.simulation_state.step_forward()
        d = super().handle(server)
        d["body"] = {}
        return d
