# Copyright (c) 2024 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Handles the custom 'bitChange' DAP request."""

from __future__ import annotations

from typing import TYPE_CHECKING, Any

import mqt.debugger

from .dap_message import DAPMessage

_TRUE_VALUES = {"1", "true", "t", "yes", "on"}
_FALSE_VALUES = {"0", "false", "f", "no", "off"}

if TYPE_CHECKING:
    from .. import DAPServer


class BitChangeDAPMessage(DAPMessage):
    """Represents the 'setVariable' (aka 'bitChange') DAP request for classical bits."""

    message_type_name: str = "setVariable"

    variables_reference: int | None
    variable_name: str
    new_value: str | bool | None

    def __init__(self, message: dict[str, Any]) -> None:
        """Initializes the 'BitChangeDAPMessage' instance.

        Args:
            message (dict[str, Any]): The object representing the 'bitChange' or 'setVariable' request.
        """
        arguments = message.get("arguments", {})
        self.variables_reference = arguments.get("variablesReference")
        self.variable_name = arguments.get("variableName") or arguments.get("name", "")
        self.new_value = arguments.get("value")
        super().__init__(message)

    def validate(self) -> None:
        """Validates the 'BitChangeDAPMessage' instance."""
        if self.variables_reference is not None and not isinstance(self.variables_reference, int):
            msg = "The 'setVariable' request requires an integer 'variablesReference' argument."
            raise ValueError(msg)
        if not isinstance(self.variable_name, str) or not self.variable_name:
            msg = "The 'bitChange' request requires a non-empty 'variableName' or 'name' argument."
            raise ValueError(msg)
        if self.new_value is not None and not isinstance(self.new_value, (bool, str)):
            msg = "The 'bitChange' request only accepts boolean or string values."
            raise ValueError(msg)

    def handle(self, server: DAPServer) -> dict[str, Any]:
        """Performs the action requested by the 'bitChange' DAP request."""
        response = super().handle(server)
        try:
            target_name = self._get_target_variable_name()
            updated_value = self._apply_change(server, target_name)
        except ValueError as exc:
            response["success"] = False
            response["message"] = str(exc)
            return response

        response["body"] = {
            "value": str(updated_value),
            "type": "boolean",
            "variablesReference": 0,
        }
        return response

    def _parse_boolean_value(self, current_value: bool) -> bool:
        if self.new_value is None:
            return not current_value
        if isinstance(self.new_value, bool):
            return self.new_value
        normalized_value = self.new_value.strip().lower()
        if normalized_value in _TRUE_VALUES:
            return True
        if normalized_value in _FALSE_VALUES:
            return False
        msg = "Only boolean values (0/1/true/false) are supported for classical bits."
        raise ValueError(msg)

    def _get_target_variable_name(self) -> str:
        if self.variables_reference is None:
            return self.variable_name
        if self.variables_reference == 1 or self.variables_reference >= 10:
            return self.variable_name
        msg = "Only classical variables can be changed."
        raise ValueError(msg)

    def _apply_change(self, server: DAPServer, name: str) -> bool:
        try:
            variable = server.simulation_state.get_classical_variable(name)
        except Exception as exc:
            msg = f"The variable '{name}' is not a classical bit."
            raise ValueError(msg) from exc

        if variable.type != mqt.debugger.VariableType.VarBool:
            msg = "Only boolean classical variables can be changed."
            raise ValueError(msg)

        current_value = bool(variable.value.bool_value)
        desired_value = self._parse_boolean_value(current_value)
        if current_value != desired_value:
            server.simulation_state.change_classical_value(name)
            variable = server.simulation_state.get_classical_variable(name)
        return bool(variable.value.bool_value)
