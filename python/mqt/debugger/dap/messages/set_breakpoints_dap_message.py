# Copyright (c) 2024 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Represents the 'setBreakpoints' DAP request."""

from __future__ import annotations

from typing import TYPE_CHECKING, Any

from .dap_message import DAPMessage

if TYPE_CHECKING:
    from .. import DAPServer


class SetBreakpointsDAPMessage(DAPMessage):
    """Represents the 'setBreakpoints' DAP request."""

    message_type_name: str = "setBreakpoints"

    breakpoints: list[tuple[int, int]]
    source: dict[str, Any]

    def __init__(self, message: dict[str, Any]) -> None:
        """Initializes the 'SetBreakpointsDAPMessage' instance.

        Args:
            message (dict[str, Any]): The object representing the 'setBreakpoints' request.
        """
        self.breakpoints = [(int(x["line"]), int(x.get("column", -1))) for x in message["arguments"]["breakpoints"]]
        self.source = message["arguments"]["source"]
        super().__init__(message)

    def validate(self) -> None:
        """Validates the 'SetBreakpointsDAPMessage' instance."""

    def handle(self, server: DAPServer) -> dict[str, Any]:
        """Performs the action requested by the 'setBreakpoints' DAP request.

        Args:
            server (DAPServer): The DAP server that received the request.

        Returns:
            dict[str, Any]: The response to the request.
        """
        if self.source["name"] != server.source_file["name"] or self.source["path"] != server.source_file["path"]:
            return self.handle_wrong_file(server)

        d = super().handle(server)

        server.simulation_state.clear_breakpoints()
        bpts = []
        for i, breakpoint_position in enumerate(self.breakpoints):
            position = server.code_coordinates_to_pos(
                breakpoint_position[0],
                breakpoint_position[1] if breakpoint_position[1] != -1 else 1 if server.columns_start_at_one else 0,
            )
            try:
                breakpoint_instruction = server.simulation_state.set_breakpoint(position)
                start, end = server.simulation_state.get_instruction_position(breakpoint_instruction)
                start_line, start_col = server.code_pos_to_coordinates(start)
                end_line, end_col = server.code_pos_to_coordinates(end)
                bpts.append({
                    "id": i,
                    "verified": True,
                    "source": self.source,
                    "line": start_line,
                    "column": start_col,
                    "endLine": end_line,
                    "endColumn": end_col,
                })
            except RuntimeError:
                bpts.append({"id": i, "verified": False, "message": "Breakpoint could not be set", "reason": "failed"})
        d["body"] = {"breakpoints": bpts}
        return d

    def handle_wrong_file(self, server: DAPServer) -> dict[str, Any]:
        """Performs the action requested by the 'setBreakpoints' DAP request when the file is wrong.

        Args:
            server (DAPServer): The DAP server that received the request.

        Returns:
            dict[str, Any]: The response to the request.
        """
        d = super().handle(server)
        bpts = []
        for i, _breakpoint in enumerate(self.breakpoints):
            bpts.append({
                "id": i,
                "verified": False,
                "message": "Breakpoints only supported in the main file",
                "reason": "failed",
            })
        d["body"] = {"breakpoints": bpts}
        return d
