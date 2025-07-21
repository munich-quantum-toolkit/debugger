# Copyright (c) 2024 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Represents the 'scopes' DAP request."""

from __future__ import annotations

from typing import TYPE_CHECKING, Any

from .dap_message import DAPMessage

if TYPE_CHECKING:
    from .. import DAPServer


class ScopesDAPMessage(DAPMessage):
    """Represents the 'scopes' DAP request."""

    message_type_name: str = "scopes"

    frame_id: int

    def __init__(self, message: dict[str, Any]) -> None:
        """Initializes the 'ScopesDAPMessage' instance.

        Args:
            message (dict[str, Any]): The object representing the 'scopes' request.
        """
        super().__init__(message)
        self.frame_id = message["arguments"]["frameId"]

    def validate(self) -> None:
        """Validates the 'ScopesDAPMessage' instance."""

    def handle(self, server: DAPServer) -> dict[str, Any]:
        """Performs the action requested by the 'scopes' DAP request.

        Args:
            server (DAPServer): The DAP server that received the request.

        Returns:
            dict[str, Any]: The response to the request.
        """
        d = super().handle(server)
        d["body"] = {"scopes": [_get_classical_scope(server), _get_quantum_state_scope(server)]}
        return d


def _get_classical_scope(server: DAPServer) -> dict[str, Any]:
    start_pos = 0
    end_pos = len(server.source_code) - 1
    start_line, start_col = server.code_pos_to_coordinates(start_pos)
    end_line, end_col = server.code_pos_to_coordinates(end_pos)
    return {
        "name": "Classical Registers",
        "presentationHint": "locals",
        "variablesReference": 1,  # Classical Registers have reference 1
        "namedVariables": server.simulation_state.get_num_classical_variables(),
        "indexedVariables": 0,
        "expensive": False,
        "source": server.source_file,
        "line": start_line,
        "column": start_col,
        "endLine": end_line,
        "endColumn": end_col,
    }


def _get_quantum_state_scope(server: DAPServer) -> dict[str, Any]:
    start_pos = 0
    end_pos = len(server.source_code) - 1
    start_line, start_col = server.code_pos_to_coordinates(start_pos)
    end_line, end_col = server.code_pos_to_coordinates(end_pos)
    return {
        "name": "Quantum State",
        "presentationHint": "registers",
        "variablesReference": 2,  # Quantum state has reference 1
        "namedVariables": 2 ** server.simulation_state.get_num_qubits(),
        "indexedVariables": 0,
        "expensive": server.simulation_state.get_num_qubits() > 5,
        "source": server.source_file,
        "line": start_line,
        "column": start_col,
        "endLine": end_line,
        "endColumn": end_col,
    }
