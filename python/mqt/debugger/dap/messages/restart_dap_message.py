# Copyright (c) 2024 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Represents the 'restart' DAP request."""

from __future__ import annotations

import locale
from pathlib import Path
from typing import TYPE_CHECKING, Any

from .dap_message import DAPMessage

if TYPE_CHECKING:
    from .. import DAPServer


class RestartDAPMessage(DAPMessage):
    """Represents the 'restart' DAP request."""

    message_type_name: str = "restart"

    no_debug: bool
    stop_on_entry: bool
    program: str

    def __init__(self, message: dict[str, Any]) -> None:
        """Initializes the 'RestartDAPMessage' instance.

        Args:
            message (dict[str, Any]): The object representing the 'restart' request.
        """
        self.no_debug = message["arguments"]["arguments"].get("noDebug", False)
        self.program = message["arguments"]["arguments"].get("program", "")
        self.stop_on_entry = message["arguments"]["arguments"].get("stopOnEntry", "")
        super().__init__(message)

    def validate(self) -> None:
        """Validates the 'LaunchDAPMessage' instance.

        Raises:
            ValueError: If the 'program' field is missing or the file does not exist.
        """
        if not self.program:
            msg = "The 'program' field is required."
            raise ValueError(msg)
        if not Path(self.program).exists():
            msg = f"The file '{self.program}' does not exist."
            raise ValueError(msg)

    def handle(self, server: DAPServer) -> dict[str, Any]:
        """Performs the action requested by the 'restart' DAP request.

        Args:
            server (DAPServer): The DAP server that received the request.

        Returns:
            dict[str, Any]: The response to the request.
        """
        server.simulation_state.reset_simulation()
        program_path = Path(self.program)
        with program_path.open("r", encoding=locale.getpreferredencoding(False)) as f:
            code = f.read()
            server.source_code = code
            server.simulation_state.load_code(code)
        if not self.stop_on_entry:
            server.simulation_state.run_simulation()
        server.source_file = {"name": program_path.name, "path": self.program}
        return {
            "type": "response",
            "request_seq": self.sequence_number,
            "success": True,
            "command": "launch",
        }
