"""Represents the custom 'highlightError' DAP event."""

from __future__ import annotations

from typing import Any

from .dap_event import DAPEvent


class HighlightError(DAPEvent):
    """Represents the 'highlightError' custom DAP event."""

    event_name = "highlightError"

    highlights: list[dict[str, Any]]
    source: dict[str, Any]

    def __init__(self, highlights: list[dict[str, Any]], source: dict[str, Any]) -> None:
        """Create a new 'highlightError' DAP event message.

        Args:
            highlights (list[dict[str, Any]]): Ranges and metadata that should be highlighted.
            source (dict[str, Any]): Information about the current source file.
        """
        self.highlights = highlights
        self.source = source
        super().__init__()

    def validate(self) -> None:
        """Validate the 'highlightError' DAP event message after creation."""

    def encode(self) -> dict[str, Any]:
        """Encode the 'highlightError' DAP event message as a dictionary."""
        encoded = super().encode()
        encoded["body"] = {"highlights": self.highlights, "source": self.source}
        return encoded
