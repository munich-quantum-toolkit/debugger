# Copyright (c) 2024 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Represents the 'variables' DAP request."""

from __future__ import annotations

from typing import TYPE_CHECKING, Any

from .dap_message import DAPMessage

if TYPE_CHECKING:
    from .. import DAPServer

# TODO: presentation hints for complex numbers


class VariablesDAPMessage(DAPMessage):
    """Represents the 'variables' DAP request."""

    message_type_name: str = "variables"

    reference: int
    filter_value: str
    start: int  # used for paging quantum states if the number of states is too large.
    count: int  # used for paging quantum states if the number of states is too large.

    def __init__(self, message: dict[str, Any]) -> None:
        """Initializes the 'VariablesDAPMessage' instance.

        Args:
            message (dict[str, Any]): The object representing the 'variables' request.
        """
        super().__init__(message)
        self.reference = message["arguments"]["variablesReference"]
        self.filter_value = message["arguments"].get("filter", "")
        self.start = message["arguments"].get("start", 0)
        self.count = message["arguments"].get("count", 0)

    def validate(self) -> None:
        """Validates the 'VariablesDAPMessage' instance."""

    def handle(self, server: DAPServer) -> dict[str, Any]:
        """Performs the action requested by the 'variables' DAP request.

        Args:
            server (DAPServer): The DAP server that received the request.

        Returns:
            dict[str, Any]: The response to the request.
        """
        d = super().handle(server)
        variables = (
            _get_classical_variables(server, self.filter_value)
            if self.reference == 1
            else _get_quantum_state_variables(server, self.start, self.count, self.filter_value)
            if self.reference == 2
            else _get_classical_children(server, self.reference - 10, self.filter_value)
            if self.reference >= 10
            else []
        )
        d["body"] = {"variables": variables}
        return d


def _get_classical_children(server: DAPServer, index: int, filter_value: str) -> list[dict[str, Any]]:
    if filter_value == "named":  # all classical children are indexed
        return []
    result = []
    num = server.simulation_state.get_num_classical_variables()
    name = server.simulation_state.get_classical_variable_name(index).split("[")[0]
    for i in range(index, num):
        n = server.simulation_state.get_classical_variable_name(i)
        if n.split("[")[0] != name:
            break
        var = server.simulation_state.get_classical_variable(n)
        result.append({
            "name": var.name,
            "evaluateName": var.name,
            "value": str(var.value.bool_value),
            "type": "boolean",
            "variablesReference": 0,
        })
    return result


def _get_classical_variables(server: DAPServer, filter_value: str) -> list[dict[str, Any]]:
    if filter_value == "indexed":  # all classical children are named
        return []
    result = []
    num = server.simulation_state.get_num_classical_variables()
    variable_groupings: dict[str, tuple[int, list[str]]] = {}
    for i in range(num):
        name = server.simulation_state.get_classical_variable_name(i)
        if "[" not in name:
            variable_groupings[name] = (i, [])
        else:
            parts = name.split("[")
            if parts[0] not in variable_groupings:
                variable_groupings[parts[0]] = (i, [])
            variable_groupings[parts[0]][1].append(name)

    for name, (first, indices) in variable_groupings.items():
        if not indices:
            var = server.simulation_state.get_classical_variable(name)
            result.append({
                "name": name,
                "evaluateName": name,
                "value": str(var.value.bool_value),
                "type": "boolean",
                "variablesReference": 0,
            })
        else:
            bitstring = ""
            for index in indices:
                var = server.simulation_state.get_classical_variable(index)
                bitstring = ("1" if var.value.bool_value else "0") + bitstring
            decimal = int(bitstring, 2)
            result.append({
                "name": name,
                "value": f"{bitstring} ({decimal})",
                "evaluateName": f"{bitstring} ({decimal})",
                "type": "integer",
                "variablesReference": 10 + first,
                # Compound registers have reference 10 + the index of their first variable
            })

    return result


def _get_quantum_state_variables(server: DAPServer, start: int, count: int, filter_value: str) -> list[dict[str, Any]]:
    if filter_value == "indexed":  # all quantum states are named
        return []
    if server.simulation_state.get_num_qubits() > 8 and count == 0:
        return [
            {
                "name": "",
                "value": "Too many qubits to display",
                "type": "string",
                "variablesReference": 0,
            }
        ]
    result = []
    num_q = server.simulation_state.get_num_qubits()
    start = 0
    count = 10
    num_variables = 2**num_q if count == 0 else count
    for i in range(start, start + num_variables):
        bitstring = format(i, f"0{num_q}b")
        result.append({
            "name": f"|{bitstring}>",
            "evaluateName": f"|{bitstring}>",
            "value": str(server.simulation_state.get_amplitude_bitstring(bitstring)),
            "type": "complex",
            "variablesReference": 0,
        })
    return result
