# Copyright (c) 2024 - 2026 Chair for Design Automation, TUM
# Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Represents the 'launch' DAP request."""

from __future__ import annotations

import contextlib
import locale
from pathlib import Path
from typing import TYPE_CHECKING, Any

import mqt.debugger

from .dap_message import DAPMessage

if TYPE_CHECKING:
    from .. import DAPServer


class LaunchDAPMessage(DAPMessage):
    """Represents the 'launch' DAP request."""

    message_type_name: str = "launch"

    no_debug: bool
    stop_on_entry: bool
    program: str

    def __init__(self, message: dict[str, Any]) -> None:
        """Initializes the 'LaunchDAPMessage' instance.

        Args:
            message (dict[str, Any]): The object representing the 'launch' request.
        """
        self.no_debug = message["arguments"].get("noDebug", False)
        self.program = message["arguments"].get("program", "")
        self.stop_on_entry = message["arguments"].get("stopOnEntry", "")
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
        """Performs the action requested by the 'launch' DAP request.

        Args:
            server (DAPServer): The DAP server that received the request.

        Returns:
            dict[str, Any]: The response to the request.
        """
        program_path = Path(self.program)
        server.source_file = {"name": program_path.name, "path": self.program}
        parsed_successfully = True
        code = program_path.read_text(encoding=locale.getpreferredencoding(False))
        server.source_code = code
        load_result = server.simulation_state.load_code(code)
        if load_result.status != mqt.debugger.LoadResultStatus.OK:
            parsed_successfully = False
            line = load_result.line if load_result.line > 0 else None
            column = load_result.column if load_result.column > 0 else None
            message = str(load_result.message or "")
            server.queue_parse_error(message, line, column)
        if parsed_successfully and not self.stop_on_entry:
            server.simulation_state.run_simulation()
        if not parsed_successfully:
            try:
                server.simulation_state.reset_simulation()
            except RuntimeError:
                pass
        return {
            "type": "response",
            "request_seq": self.sequence_number,
            "success": True,
            "command": "launch",
        }
