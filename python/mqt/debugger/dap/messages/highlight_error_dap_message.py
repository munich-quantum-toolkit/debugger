# Copyright (c) 2024 - 2026 Chair for Design Automation, TUM
# Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Represents the custom 'highlightError' DAP event."""

from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any

from .dap_event import DAPEvent

if TYPE_CHECKING:
    from collections.abc import Sequence


class HighlightError(DAPEvent):
    """Represents the 'highlightError' custom DAP event.

    Attributes:
        event_name (str): DAP event identifier emitted by this message.
        highlights (list[dict[str, Any]]): Normalized highlight entries with ranges and metadata.
        source (dict[str, Any]): Normalized DAP source information for the highlighted file.
    """

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
        """Validate the 'highlightError' DAP event message after creation.

        Raises:
            ValueError: If required highlight fields are missing or empty.
        """
        if not self.highlights:
            msg = "At least one highlight entry is required to show the issue location."
            raise ValueError(msg)

        for highlight in self.highlights:
            if "message" not in highlight or not str(highlight["message"]).strip():
                msg = "Each highlight entry must contain a descriptive 'message'."
                raise ValueError(msg)

    def encode(self) -> dict[str, Any]:
        """Encode the 'highlightError' DAP event message as a dictionary.

        Returns:
            dict[str, Any]: The encoded DAP event payload.
        """
        encoded = super().encode()
        encoded["body"] = {"highlights": self.highlights, "source": self.source}
        return encoded

    @staticmethod
    def _normalize_highlight(entry: Mapping[str, Any]) -> dict[str, Any]:
        """Return a shallow copy of a highlight entry with guaranteed structure.

        Args:
            entry (Mapping[str, Any]): Highlight metadata including a range mapping.

        Returns:
            dict[str, Any]: A normalized highlight entry suitable for serialization.

        Raises:
            TypeError: If the range mapping or its positions are not mappings.
            ValueError: If required fields are missing or malformed.
        """
        if "range" not in entry:
            msg = "A highlight entry must contain a 'range'."
            raise ValueError(msg)
        highlight_range = entry["range"]
        if not isinstance(highlight_range, Mapping):
            msg = "Highlight range must be a mapping with 'start' and 'end'."
            raise TypeError(msg)

        start = HighlightError._normalize_position(highlight_range.get("start"))
        end = HighlightError._normalize_position(highlight_range.get("end"))
        if HighlightError._start_comes_after_end(start, end):
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
        """Normalize a position mapping, ensuring it includes a line and column.

        Args:
            position (Mapping[str, Any] | None): The position mapping to normalize.

        Returns:
            dict[str, int]: A normalized position with integer line and column.

        Raises:
            TypeError: If the provided position is not a mapping.
            ValueError: If required keys are missing.
        """
        if not isinstance(position, Mapping):
            msg = "Highlight positions must be mappings with 'line' and 'column'."
            raise TypeError(msg)
        try:
            line = int(position["line"])
            column = int(position["column"])
        except KeyError as exc:
            msg = "Highlight positions require 'line' and 'column'."
            raise ValueError(msg) from exc
        return {
            "line": line,
            "column": column,
        }

    @staticmethod
    def _normalize_source(source: Mapping[str, Any] | None) -> dict[str, Any]:
        """Create a defensive copy of the provided DAP Source information.

        Args:
            source (Mapping[str, Any] | None): The source mapping to normalize.

        Returns:
            dict[str, Any]: Normalized source information with string fields.

        Raises:
            TypeError: If the source is not a mapping.
            ValueError: If required keys are missing.
        """
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
    def _start_comes_after_end(start: Mapping[str, Any], end: Mapping[str, Any]) -> bool:
        """Return True if 'start' describes a position after 'end'.

        Args:
            start (Mapping[str, Any]): The start position mapping.
            end (Mapping[str, Any]): The end position mapping.

        Returns:
            bool: True when the start position is after the end position.
        """
        start_line = int(start.get("line", 0))
        start_column = int(start.get("column", 0))
        end_line = int(end.get("line", 0))
        end_column = int(end.get("column", 0))
        return (end_line, end_column) < (start_line, start_column)
