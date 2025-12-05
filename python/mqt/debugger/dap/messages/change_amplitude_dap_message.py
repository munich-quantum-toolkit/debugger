# Copyright (c) 2024 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Handles edits of quantum amplitudes via the DAP ``setVariable`` request."""

from __future__ import annotations

from dataclasses import dataclass
from typing import TYPE_CHECKING, Any, cast

import mqt.debugger

from .dap_message import DAPMessage

if TYPE_CHECKING:
    from .. import DAPServer

_QUANTUM_REFERENCE = 2
_EPS = 1e-9


@dataclass
class _TargetAmplitude:
    """Container describing the requested bitstring and desired complex value."""

    bitstring: str
    desired: complex


def _format_complex(value: mqt.debugger.Complex) -> str:
    """Represent a complex number in the format expected by Visual Studio Code."""
    real = value.real
    imag = value.imaginary
    sign = "+" if imag >= 0 else "-"
    return f"{real:.6g} {sign} {abs(imag):.6g}i"


def _complex_matches(current: mqt.debugger.Complex, desired: complex) -> bool:
    """Return ``True`` if the debugger amplitude equals the desired value."""
    return abs(current.real - desired.real) <= _EPS and abs(current.imaginary - desired.imag) <= _EPS


def _normalize_value(value: str) -> str:
    """Normalize DAP complex literals so Python's :func:`complex` can parse them.

    Visual Studio Code currently sends amplitudes in the ``a+bi`` / ``a-bi`` form,
    but also accepts real-only (``a``) or imaginary-only (``bi``) literals with
    arbitrary whitespace. Plain ``i``/``-i`` or inputs mixing ``i`` and ``j`` are
    intentionally unsupported. When a literal contains ``i`` but no ``j``, this
    function rewrites ``i`` to ``j`` so Python understands the imaginary unit.

    Args:
        value (str): Raw value received from the DAP client.

    Returns:
        str: Normalized literal accepted by ``complex``.
    """
    normalized = value.strip().replace(" ", "")
    if not normalized:
        msg = (
            "The new amplitude value must not be empty; use literals such as "
            "'a+bi', 'a-bi', 'a', or 'bi'. Plain 'i'/'-i' and mixed 'i'/'j' "
            "inputs are rejected, and 'i' is only converted to 'j' when no 'j' is present."
        )
        raise ValueError(msg)
    # Visual Studio Code allows using `i` as the imaginary unit. Python expects `j`.
    if "i" in normalized and "j" not in normalized:
        normalized = normalized.replace("i", "j")
    return normalized


class AmplitudeChangeDAPMessage(DAPMessage):
    """Represents the 'setVariable' DAP request for quantum amplitude edits."""

    message_type_name: str = "setVariable"

    variables_reference: int | None
    variable_name: Any
    new_value: str | None

    def __init__(self, message: dict[str, Any]) -> None:
        """Initialize the amplitude 'setVariable' handler instance.

        Args:
            message (dict[str, Any]): The raw DAP request.
        """
        arguments = message.get("arguments", {})
        self.variables_reference = arguments.get("variablesReference")
        self.variable_name = arguments.get("variableName") or arguments.get("name", "")
        raw_value = arguments.get("value")
        self.new_value = raw_value if isinstance(raw_value, str) else None
        super().__init__(message)

    def validate(self) -> None:
        """Validate that the request targets amplitudes and provides a new value."""
        if self.variables_reference != _QUANTUM_REFERENCE:
            msg = "This handler only supports quantum amplitudes."
            raise ValueError(msg)
        if not isinstance(self.variable_name, str) or not self.variable_name:
            msg = "The 'setVariable' request requires a non-empty 'variableName' argument."
            raise ValueError(msg)
        if self.new_value is None:
            msg = "The 'setVariable' request for quantum amplitudes must provide the new complex value as a string."
            raise ValueError(msg)

    def handle(self, server: DAPServer) -> dict[str, Any]:
        """Apply the amplitude change and return the new complex value.

        Args:
            server (DAPServer): The DAP server handling the request.

        Returns:
            dict[str, Any]: The DAP response with the updated value.
        """
        response = super().handle(server)
        try:
            target = self._parse_request(server)
            updated = self._apply_change(server, target)
        except ValueError as exc:
            response["success"] = False
            response["message"] = str(exc)
            return response

        response["body"] = {
            "value": _format_complex(updated),
            "type": "complex",
            "variablesReference": 0,
        }
        return response

    def _parse_request(self, server: DAPServer) -> _TargetAmplitude:
        """Extract the targeted bitstring and desired complex amplitude.

        Args:
            server (DAPServer): The DAP server providing simulator metadata.

        Returns:
            _TargetAmplitude: Bitstring and target value requested by VS Code.
        """
        bitstring = self._extract_bitstring()
        if len(bitstring) != server.simulation_state.get_num_qubits():
            msg = f"The bitstring '{bitstring}' must have length {server.simulation_state.get_num_qubits()}."
            raise ValueError(msg)
        normalized = _normalize_value(self.new_value or "")
        try:
            desired = complex(normalized)
        except ValueError as exc:
            msg = f"The provided value '{self.new_value}' is not a valid complex number."
            raise ValueError(msg) from exc
        return _TargetAmplitude(bitstring, desired)

    def _extract_bitstring(self) -> str:
        """Return the ``|...>`` bitstring referenced in the request.

        Returns:
            str: The computational basis state name without delimiters.
        """
        name = cast("str", self.variable_name).strip()
        if not name.startswith("|") or not name.endswith(">"):
            msg = "Quantum amplitudes must be addressed using the '|...>' notation."
            raise ValueError(msg)
        bitstring = name[1:-1]
        if not bitstring or any(ch not in "01" for ch in bitstring):
            msg = f"'{self.variable_name}' is not a valid computational basis state."
            raise ValueError(msg)
        return bitstring

    @staticmethod
    def _apply_change(server: DAPServer, target: _TargetAmplitude) -> mqt.debugger.Complex:
        """Write the requested amplitude into the simulation state if needed.

        Args:
            server (DAPServer): The DAP server providing simulator access.
            target (_TargetAmplitude): The desired bitstring/value pair.

        Returns:
            mqt.debugger.Complex: The amplitude returned by the simulator after the update.
        """
        current = server.simulation_state.get_amplitude_bitstring(target.bitstring)
        if _complex_matches(current, target.desired):
            return current

        desired_value = mqt.debugger.Complex(target.desired.real, target.desired.imag)
        try:
            server.simulation_state.change_amplitude_value(target.bitstring, desired_value)
        except RuntimeError as exc:
            msg = str(exc)
            raise ValueError(msg) from exc
        return server.simulation_state.get_amplitude_bitstring(target.bitstring)
