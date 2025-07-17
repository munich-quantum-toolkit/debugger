# Copyright (c) 2024 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Represents the 'exceptionInfo' DAP request."""

from __future__ import annotations

from typing import TYPE_CHECKING, Any

from .dap_message import DAPMessage

if TYPE_CHECKING:
    from .. import DAPServer

ASSERTION_DESCRIPTIONS = {
    "assert-ent": "The given qubits are not in an entangled state.",
    "assert-sup": "The given qubits are not in a superposition.",
    "assert-eq": "The given quantum states are not within the given tolerance.",
}


class ExceptionInfoDAPMessage(DAPMessage):
    """Represents the 'exceptionInfo' DAP request."""

    message_type_name: str = "exceptionInfo"

    def __init__(self, message: dict[str, Any]) -> None:
        """Initializes the 'ExceptionInfoDAPMessage' instance.

        Args:
            message (dict[str, Any]): The object representing the 'exceptionInfo' request.
        """
        super().__init__(message)

    def validate(self) -> None:
        """Validates the 'ExceptionInfoDAPMessage' instance."""

    def handle(self, server: DAPServer) -> dict[str, Any]:
        """Performs the action requested by the 'exceptionInfo' DAP request.

        Args:
            server (DAPServer): The DAP server that received the request.

        Returns:
            dict[str, Any]: The response to the request.
        """
        previous_instruction = server.simulation_state.get_current_instruction()
        (start, end) = server.simulation_state.get_instruction_position(previous_instruction)
        instruction = server.source_code[start:end]
        assertion_type = next(x for x in ("assert-ent", "assert-sup", "assert-eq") if x in instruction)
        d = super().handle(server)
        d["body"] = {
            "exceptionId": instruction.strip(),
            "breakMode": "always",
            "description": ASSERTION_DESCRIPTIONS[assertion_type],
            "details": {
                "typeName": assertion_type,
            },
        }
        return d
