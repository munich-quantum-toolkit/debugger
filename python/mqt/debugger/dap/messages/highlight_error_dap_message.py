"""Represents the custom 'highlightError' DAP request."""

from __future__ import annotations

from typing import TYPE_CHECKING, Any

from mqt.debugger import ErrorCauseType

from .dap_message import DAPMessage

if TYPE_CHECKING:
    from .. import DAPServer


class HighlightError(DAPMessage):
    """Represents the 'highlightError' custom DAP request."""

    message_type_name = "highlightError"

    def __init__(self, message: dict[str, Any]) -> None:
        """Initialize the 'highlightError' request instance.

        Args:
            message (dict[str, Any]): The object representing the request.
        """
        arguments = message.get("arguments", {}) or {}
        self._arguments = arguments
        self._instruction_filter = arguments.get("instruction")
        super().__init__(message)

    def validate(self) -> None:
        """Validate the provided request arguments."""
        if not isinstance(self._arguments, dict):
            msg = "The 'arguments' property must be an object."
            raise ValueError(msg)
        if self._instruction_filter is not None and not isinstance(self._instruction_filter, int):
            msg = "The 'instruction' argument must be an integer if provided."
            raise ValueError(msg)

    def handle(self, server: DAPServer) -> dict[str, Any]:
        """Execute the request and return diagnostic highlights.

        Args:
            server (DAPServer): The server handling the request.

        Returns:
            dict[str, Any]: The response containing highlight metadata.
        """
        response = super().handle(server)
        response["body"] = {
            "highlights": self._collect_highlights(server),
            "source": getattr(server, "source_file", {}) or {},
        }
        return response

    def _collect_highlights(self, server: DAPServer) -> list[dict[str, Any]]:
        """Collect highlight information from the diagnostics backend."""
        if not getattr(server, "source_code", ""):
            return []

        try:
            diagnostics = server.simulation_state.get_diagnostics()
            error_causes = diagnostics.potential_error_causes()
        except RuntimeError:
            return []

        highlights: list[dict[str, Any]] = []
        for cause in error_causes:
            if self._instruction_filter is not None and cause.instruction != self._instruction_filter:
                continue

            highlight = self._encode_highlight(server, cause)
            if highlight is not None:
                highlights.append(highlight)

        return highlights

    def _encode_highlight(self, server: DAPServer, cause: Any) -> dict[str, Any] | None:
        """Encode a single highlight entry for the response payload."""
        try:
            start_pos, end_pos = server.simulation_state.get_instruction_position(cause.instruction)
        except RuntimeError:
            return None

        message = server.format_error_cause(cause)
        start_line, start_column = server.code_pos_to_coordinates(start_pos)
        end_line, end_column = server.code_pos_to_coordinates(end_pos)
        snippet = server.source_code[start_pos : end_pos + 1].replace("\r", "")

        return {
            "instruction": int(cause.instruction),
            "range": {
                "start": {"line": start_line, "column": start_column},
                "end": {"line": end_line, "column": end_column},
            },
            "reason": self._format_reason(cause.type),
            "code": snippet.strip(),
            "message": message,
        }

    def _format_reason(self, cause_type: ErrorCauseType) -> str:
        """Format the cause type into a short identifier for clients."""
        if cause_type == ErrorCauseType.MissingInteraction:
            return "missingInteraction"
        if cause_type == ErrorCauseType.ControlAlwaysZero:
            return "controlAlwaysZero"
        return "unknown"
