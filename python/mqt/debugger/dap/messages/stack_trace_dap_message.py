# Copyright (c) 2024 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Represents the 'stackTrace' DAP request."""

from __future__ import annotations

from typing import TYPE_CHECKING, Any

from .dap_message import DAPMessage

if TYPE_CHECKING:
    from .. import DAPServer


class StackTraceDAPMessage(DAPMessage):
    """Represents the 'stackTrace' DAP request."""

    message_type_name: str = "stackTrace"

    def __init__(self, message: dict[str, Any]) -> None:
        """Initializes the 'StackTraceDAPMessage' instance.

        Args:
            message (dict[str, Any]): The object representing the 'stackTrace' request.
        """
        super().__init__(message)

    def validate(self) -> None:
        """Validates the 'StackTraceDAPMessage' instance."""

    def handle(self, server: DAPServer) -> dict[str, Any]:
        """Performs the action requested by the 'stackTrace' DAP request.

        Args:
            server (DAPServer): The DAP server that received the request.

        Returns:
            dict[str, Any]: The response to the request.
        """
        d = super().handle(server)

        stack_frames = []
        depth = server.simulation_state.get_stack_depth()
        stack_trace = server.simulation_state.get_stack_trace(depth)
        for i, frame in enumerate(stack_trace):
            (start, end) = server.simulation_state.get_instruction_position(frame)
            start_line, start_col = server.code_pos_to_coordinates(start)
            end_line, end_col = server.code_pos_to_coordinates(end)
            if i == len(stack_trace) - 1:
                name = "main"
            else:
                parent_instr = stack_trace[i + 1]
                (parent_start, parent_end) = server.simulation_state.get_instruction_position(parent_instr)
                name = server.source_code[parent_start:parent_end].strip().split(" ")[0].strip()
            stack_frames.append({
                "id": depth - i,
                "name": name,
                "line": start_line,
                "endLine": end_line,
                "column": start_col,
                "endColumn": end_col,
                "source": server.source_file,
            })

        d["body"] = {"stackFrames": stack_frames, "totalFrames": depth}
        return d
