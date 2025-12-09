# Copyright (c) 2024 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Represents the custom 'highlightError' DAP event."""
from __future__ import annotations

from typing import Any, Mapping, Sequence

from .dap_event import DAPEvent

class HighlightError(DAPEvent):
    """Represents the 'highlightError' custom DAP event."""

    event_name = "highlightError"

    highlights: list[dict[str, Any]]
    source: dict[str, Any]

    def __init__(self, highlights: Sequence[Mapping[str, Any]], source: Mapping[str, Any]) -> None:
        """Create a new 'highlightError' DAP event message.

        Args:
            highlights (Sequence[Mapping[str, Any]]): Highlight entries describing the problematic ranges.
            source (Mapping[str, Any]): Information about the current source file.
        """
        self.highlights = [self._normalize_highlight(entry) for entry in highlights]
        self.source = self._normalize_source(source)
        super().__init__()

    def validate(self) -> None:
        """Validate the 'highlightError' DAP event message after creation."""
        if not self.highlights:
            msg = "At least one highlight entry is required to show the issue location."
            raise ValueError(msg)

        for highlight in self.highlights:
            if "message" not in highlight or not str(highlight["message"]).strip():
                msg = "Each highlight entry must contain a descriptive 'message'."
                raise ValueError(msg)
            highlight_range = highlight.get("range")
            if not isinstance(highlight_range, dict):
                msg = "Each highlight entry must provide a 'range' dictionary."
                raise ValueError(msg)
            start = highlight_range.get("start")
            end = highlight_range.get("end")
            if not isinstance(start, dict) or not isinstance(end, dict):
                msg = "Highlight ranges must define 'start' and 'end' coordinates."
                raise ValueError(msg)
            if self._later_than(start, end):
                msg = "Highlight range 'end' must not precede 'start'."
                raise ValueError(msg)

        if "name" not in self.source or "path" not in self.source:
            msg = "Source information must at least expose 'name' and 'path'."
            raise ValueError(msg)

    def encode(self) -> dict[str, Any]:
        """Encode the 'highlightError' DAP event message as a dictionary."""
        encoded = super().encode()
        encoded["body"] = {"highlights": self.highlights, "source": self.source}
        return encoded

    @staticmethod
    def _normalize_highlight(entry: Mapping[str, Any]) -> dict[str, Any]:
        """Return a shallow copy of a highlight entry with guaranteed structure."""
        if "range" not in entry:
            msg = "A highlight entry must contain a 'range'."
            raise ValueError(msg)
        highlight_range = entry["range"]
        if not isinstance(highlight_range, Mapping):
            msg = "Highlight range must be a mapping with 'start' and 'end'."
            raise TypeError(msg)

        start = HighlightError._normalize_position(highlight_range.get("start"))
        end = HighlightError._normalize_position(highlight_range.get("end"))
        if HighlightError._later_than(start, end):
            msg = "Highlight range 'end' must be after 'start'."
            raise ValueError(msg)

        normalized = dict(entry)
        normalized["instruction"] = int(normalized.get("instruction", -1))
        normalized["reason"] = str(normalized.get("reason", "unknown"))
        normalized["code"] = str(normalized.get("code", ""))
        normalized["message"] = str(normalized.get("message", "")).strip()
        normalized["range"] = {
            "start": start,
            "end": end,
        }
        return normalized

    @staticmethod
    def _normalize_position(position: Mapping[str, Any] | None) -> dict[str, int]:
        """Normalize a position mapping, ensuring it includes a line and column."""
        if not isinstance(position, Mapping):
            msg = "Highlight positions must be mappings with 'line' and 'column'."
            raise TypeError(msg)
        try:
            line = int(position["line"])
            column = int(position["column"])
        except KeyError as exc:
            raise ValueError("Highlight positions require 'line' and 'column'.") from exc
        return {
            "line": line,
            "column": column,
        }

    @staticmethod
    def _normalize_source(source: Mapping[str, Any]) -> dict[str, Any]:
        """Create a defensive copy of the provided DAP Source information."""
        if not isinstance(source, Mapping):
            msg = "Source information must be provided as a mapping."
            raise TypeError(msg)
        normalized = dict(source)
        if "name" not in normalized or "path" not in normalized:
            msg = "Source mappings must at least provide 'name' and 'path'."
            raise ValueError(msg)
        normalized["name"] = str(normalized["name"])
        normalized["path"] = str(normalized["path"])
        return normalized

    @staticmethod
    def _later_than(start: Mapping[str, Any], end: Mapping[str, Any]) -> bool:
        """Return True if 'end' describes a position before 'start'."""
        start_line = int(start.get("line", 0))
        start_column = int(start.get("column", 0))
        end_line = int(end.get("line", 0))
        end_column = int(end.get("column", 0))
        return (end_line, end_column) < (start_line, start_column)