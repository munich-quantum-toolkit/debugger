# Copyright (c) 2024 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Represents the 'initialize' DAP request."""

from __future__ import annotations

from typing import TYPE_CHECKING, Any

import mqt.debugger

from .dap_message import DAPMessage
from .utils import get_default_capabilities

if TYPE_CHECKING:
    from .. import DAPServer


class InitializeDAPMessage(DAPMessage):
    """Represents the 'initialize' DAP request."""

    message_type_name: str = "initialize"

    client_id: str
    client_name: str
    adapter_id: str
    path_format: str
    lines_start_at1: bool
    columns_start_at1: bool

    def __init__(self, message: dict[str, Any]) -> None:
        """Initializes the 'InitializeDAPMessage' instance.

        Args:
            message (dict[str, Any]): The object representing the 'initialize' request.
        """
        self.client_id = message["arguments"].get("clientID", "")
        self.client_name = message["arguments"].get("clientName", "")
        self.adapter_id = message["arguments"].get("adapterID", "")
        self.path_format = message["arguments"].get("pathFormat", "")
        self.lines_start_at1 = message["arguments"].get("linesStartAt1", True)
        self.columns_start_at1 = message["arguments"].get("columnsStartAt1", True)
        super().__init__(message)

    def validate(self) -> None:
        """Validates the 'InitializeDAPMessage' instance.

        Raises:
            ValueError: If the adapter ID is not `mqtqasm`.
        """
        if self.adapter_id != "mqtqasm":
            msg = f"Adapter ID must be `mqtqasm`, was {self.adapter_id}"
            raise ValueError(msg)

    def handle(self, server: DAPServer) -> dict[str, Any]:
        """Performs the action requested by the 'initialize' DAP request.

        Args:
            server (DAPServer): The DAP server that received the request.

        Returns:
            dict[str, Any]: The response to the request.
        """
        server.columns_start_at_one = self.columns_start_at1
        server.lines_start_at_one = self.lines_start_at1
        server.simulation_state = mqt.debugger.create_ddsim_simulation_state()
        return {
            "type": "response",
            "request_seq": self.sequence_number,
            "success": True,
            "command": "initialize",
            "body": get_default_capabilities(),
        }
