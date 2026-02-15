# Copyright (c) 2024 - 2026 Chair for Design Automation, TUM
# Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Tests for the DAP server helper utilities."""

from __future__ import annotations

from types import SimpleNamespace

from mqt.debugger.dap.dap_server import DAPServer


def test_code_pos_to_coordinates_handles_line_end() -> None:
    """Ensure coordinates for newline positions stay on the current line."""
    server = DAPServer()
    server.source_code = "measure q[0] -> c[0];\nmeasure q[1] -> c[1];\n"
    line, column = server.code_pos_to_coordinates(server.source_code.index("\n"))
    assert line == 1
    # Column is 1-based because the DAP client requests it that way.
    assert column == len("measure q[0] -> c[0];") + 1


def test_build_highlight_entry_does_not_span_next_instruction() -> None:
    """Ensure highlight ranges stop at the end of the instruction."""
    server = DAPServer()
    server.source_code = "measure q[0] -> c[0];\nmeasure q[1] -> c[1];\n"
    first_line_end = server.source_code.index("\n")
    fake_diagnostics = SimpleNamespace(potential_error_causes=list)
    fake_state = SimpleNamespace(
        get_instruction_position=lambda _instr: (0, first_line_end),
        get_diagnostics=lambda: fake_diagnostics,
    )
    server.simulation_state = fake_state  # type: ignore[assignment]

    entries = server.collect_highlight_entries(0)
    assert entries
    entry = entries[0]
    assert entry["range"]["start"]["line"] == 1
    assert entry["range"]["end"]["line"] == 1
