"""Handles edits of quantum amplitudes via the DAP ``setVariable`` request."""

from __future__ import annotations

from dataclasses import dataclass
from typing import TYPE_CHECKING, Any

import mqt.debugger

from .dap_message import DAPMessage

if TYPE_CHECKING:
    from .. import DAPServer

_QUANTUM_REFERENCE = 2
_EPS = 1e-9

@dataclass
class _TargetAmplitude:
    bitstring: str
    desired: complex


def _format_complex(value: mqt.debugger.Complex) -> str:
    real = value.real
    imag = value.imaginary
    sign = "+" if imag >= 0 else "-"
    return f"{real:.6g} {sign} {abs(imag):.6g}i"


def _complex_matches(current: mqt.debugger.Complex, desired: complex) -> bool:
    return abs(current.real - desired.real) <= _EPS and abs(current.imaginary - desired.imag) <= _EPS


def _normalize_value(value: str) -> str:
    """Normalize complex literals so that Python's complex() can parse them."""
    normalized = value.strip().replace(" ", "")
    if not normalized:
        msg = "The new amplitude value must not be empty."
        raise ValueError(msg)
    # Visual Studio Code allows using `i` as the imaginary unit. Python expects `j`.
    if "i" in normalized and "j" not in normalized:
        normalized = normalized.replace("i", "j")
    return normalized


class AmplitudeChangeDAPMessage(DAPMessage):
    """Represents the 'setVariable' DAP request for quantum amplitude edits."""

    message_type_name: str = "setVariable"

    variables_reference: int | None
    variable_name: str
    new_value: str | None

    def __init__(self, message: dict[str, Any]) -> None:
        arguments = message.get("arguments", {})
        self.variables_reference = arguments.get("variablesReference")
        self.variable_name = arguments.get("variableName") or arguments.get("name", "")
        raw_value = arguments.get("value")
        self.new_value = raw_value if isinstance(raw_value, str) else None
        super().__init__(message)

    def validate(self) -> None:
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
        bitstring = self._extract_bitstring()
        if len(bitstring) != server.simulation_state.get_num_qubits():
            msg = f"The bitstring '{bitstring}' must have length {server.simulation_state.get_num_qubits()}."
            raise ValueError(msg)
        normalized = _normalize_value(self.new_value or "")
        try:
            desired = complex(normalized)
        except ValueError as exc:  # noqa: BLE001
            msg = f"The provided value '{self.new_value}' is not a valid complex number."
            raise ValueError(msg) from exc
        return _TargetAmplitude(bitstring, desired)

    def _extract_bitstring(self) -> str:
        name = self.variable_name.strip()
        if not name.startswith("|") or not name.endswith(">"):
            msg = "Quantum amplitudes must be addressed using the '|...>' notation."
            raise ValueError(msg)
        bitstring = name[1:-1]
        if not bitstring or any(ch not in "01" for ch in bitstring):
            msg = f"'{self.variable_name}' is not a valid computational basis state."
            raise ValueError(msg)
        return bitstring

    def _apply_change(self, server: DAPServer, target: _TargetAmplitude) -> mqt.debugger.Complex:
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
