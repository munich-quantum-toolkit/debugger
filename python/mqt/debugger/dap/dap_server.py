# Copyright (c) 2024 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Handles the responsibilities for a DAP server."""

from __future__ import annotations

import json
import re
import socket
import sys
from typing import TYPE_CHECKING, Any

import mqt.debugger

from .messages import (
    AmplitudeChangeDAPMessage,
    BitChangeDAPMessage,
    ConfigurationDoneDAPMessage,
    ContinueDAPMessage,
    DisconnectDAPMessage,
    ExceptionInfoDAPMessage,
    InitializeDAPMessage,
    LaunchDAPMessage,
    NextDAPMessage,
    PauseDAPMessage,
    RestartDAPMessage,
    RestartFrameDAPMessage,
    ReverseContinueDAPMessage,
    ScopesDAPMessage,
    SetBreakpointsDAPMessage,
    SetExceptionBreakpointsDAPMessage,
    StackTraceDAPMessage,
    StepBackDAPMessage,
    StepInDAPMessage,
    StepOutDAPMessage,
    TerminateDAPMessage,
    ThreadsDAPMessage,
    VariablesDAPMessage,
)
from .messages.change_amplitude_dap_message import _QUANTUM_REFERENCE

if TYPE_CHECKING:
    from .messages import Request

supported_messages: list[type[Request]] = [
    InitializeDAPMessage,
    DisconnectDAPMessage,
    LaunchDAPMessage,
    SetBreakpointsDAPMessage,
    ThreadsDAPMessage,
    StackTraceDAPMessage,
    ConfigurationDoneDAPMessage,
    NextDAPMessage,
    StepBackDAPMessage,
    StepInDAPMessage,
    ContinueDAPMessage,
    TerminateDAPMessage,
    RestartDAPMessage,
    ScopesDAPMessage,
    VariablesDAPMessage,
    BitChangeDAPMessage,
    AmplitudeChangeDAPMessage,
    ReverseContinueDAPMessage,
    StepOutDAPMessage,
    PauseDAPMessage,
    SetExceptionBreakpointsDAPMessage,
    ExceptionInfoDAPMessage,
    RestartFrameDAPMessage,
]


def send_message(msg: str, client: socket.socket) -> None:
    """Send a message to the client according to the DAP messaging protocol.

    Args:
        msg (str): The message to send.
        client (socket.socket): The client socket to send the message to.
    """
    msg = msg.replace("\n", "\r\n")
    length = len(msg)
    header = f"Content-Length: {length}\r\n\r\n".encode("ascii")
    client.sendall(header + msg.encode("utf-8"))


class DAPServer:
    """The DAP server class."""

    host: str
    port: int

    simulation_state: mqt.debugger.SimulationState
    source_file: dict[str, Any]
    source_code: str
    can_step_back: bool
    exception_breakpoints: list[str]
    lines_start_at_one: bool
    columns_start_at_one: bool

    def __init__(self, host: str = "127.0.0.1", port: int = 4711) -> None:
        """Create a new DAP server instance.

        Args:
            host (str, optional): The host IP Address. Defaults to "0.0.0.0".
            port (int, optional): The port to run the server on. Defaults to 4711.
        """
        self.host = host
        self.port = port
        self.can_step_back = False
        self.simulation_state = mqt.debugger.SimulationState()
        self.lines_start_at_one = True
        self.columns_start_at_one = True
        self.pending_highlights: list[dict[str, Any]] = []
        self._prevent_exit = False

    def start(self) -> None:
        """Start the DAP server and listen for one connection."""
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            try:
                s.bind((self.host, self.port))
            except OSError:
                print("Address already in use")  # noqa: T201
                return

            print("Initialization complete")  # noqa: T201
            sys.stdout.flush()  # we need to flush stdout so  that the client can read the message

            s.listen()

            try:
                conn, _addr = s.accept()
                with conn:
                    self.handle_client(conn)
            except RuntimeError:
                s.close()

    def handle_client(self, connection: socket.socket) -> None:
        """Handle incoming messages from the client.

        Args:
            connection (socket.socket): The client socket.
        """
        data_str = ""
        message_str = ""
        while True:
            if not message_str or not data_str:
                data = connection.recv(1024)
                data_str += data.decode()
            first_end = data_str.find("Content-Length:", 1)
            if first_end != -1:
                message_str = data_str[:first_end]
                data_str = data_str[first_end:]
            elif data_str.count("{") == data_str.count("}"):
                message_str = data_str
                data_str = ""
            else:
                message_str = ""
                continue
            parts = message_str.split("\n")
            if not parts or not data:
                break
            payload = json.loads(parts[-1])
            result, cmd = self.handle_command(payload)
            result_payload = json.dumps(result)
            send_message(result_payload, connection)
            if isinstance(
                cmd,
                (
                    mqt.debugger.dap.messages.NextDAPMessage,
                    mqt.debugger.dap.messages.StepBackDAPMessage,
                    mqt.debugger.dap.messages.StepInDAPMessage,
                    mqt.debugger.dap.messages.StepOutDAPMessage,
                    mqt.debugger.dap.messages.ContinueDAPMessage,
                    mqt.debugger.dap.messages.ReverseContinueDAPMessage,
                    mqt.debugger.dap.messages.RestartFrameDAPMessage,
                    mqt.debugger.dap.messages.RestartDAPMessage,
                    mqt.debugger.dap.messages.LaunchDAPMessage,
                ),
            ):
                self._prevent_exit = False

            e: mqt.debugger.dap.messages.DAPEvent | None = None
            if isinstance(cmd, mqt.debugger.dap.messages.LaunchDAPMessage):
                e = mqt.debugger.dap.messages.InitializedDAPEvent()
                event_payload = json.dumps(e.encode())
                send_message(event_payload, connection)
            if (
                isinstance(
                    cmd, (mqt.debugger.dap.messages.LaunchDAPMessage, mqt.debugger.dap.messages.RestartDAPMessage)
                )
                and cmd.stop_on_entry
            ):
                e = mqt.debugger.dap.messages.StoppedDAPEvent(
                    mqt.debugger.dap.messages.StopReason.ENTRY, "Stopped on entry"
                )
                event_payload = json.dumps(e.encode())
                send_message(event_payload, connection)
            if isinstance(
                cmd,
                (
                    mqt.debugger.dap.messages.NextDAPMessage,
                    mqt.debugger.dap.messages.StepBackDAPMessage,
                    mqt.debugger.dap.messages.StepInDAPMessage,
                    mqt.debugger.dap.messages.StepOutDAPMessage,
                    mqt.debugger.dap.messages.ContinueDAPMessage,
                    mqt.debugger.dap.messages.ReverseContinueDAPMessage,
                    mqt.debugger.dap.messages.RestartFrameDAPMessage,
                ),
            ) or (
                isinstance(
                    cmd,
                    (
                        mqt.debugger.dap.messages.LaunchDAPMessage,
                        mqt.debugger.dap.messages.RestartDAPMessage,
                    ),
                )
                and not cmd.stop_on_entry
            ):
                event = (
                    mqt.debugger.dap.messages.StopReason.EXCEPTION
                    if self.simulation_state.did_assertion_fail()
                    else mqt.debugger.dap.messages.StopReason.BREAKPOINT_INSTRUCTION
                    if self.simulation_state.was_breakpoint_hit()
                    else mqt.debugger.dap.messages.StopReason.STEP
                )
                message = (
                    "An assertion failed"
                    if self.simulation_state.did_assertion_fail()
                    else "Stopped at breakpoint"
                    if self.simulation_state.was_breakpoint_hit()
                    else "Stopped after step"
                )
                e = mqt.debugger.dap.messages.StoppedDAPEvent(event, message)
                event_payload = json.dumps(e.encode())
                send_message(event_payload, connection)
                if self.simulation_state.did_assertion_fail():
                    self.handle_assertion_fail(connection)
            if isinstance(cmd, mqt.debugger.dap.messages.TerminateDAPMessage):
                e = mqt.debugger.dap.messages.TerminatedDAPEvent()
                event_payload = json.dumps(e.encode())
                send_message(event_payload, connection)
                e = mqt.debugger.dap.messages.ExitedDAPEvent(143)
                event_payload = json.dumps(e.encode())
                send_message(event_payload, connection)
            if isinstance(cmd, mqt.debugger.dap.messages.PauseDAPMessage):
                e = mqt.debugger.dap.messages.StoppedDAPEvent(
                    mqt.debugger.dap.messages.StopReason.PAUSE, "Stopped after pause"
                )
                event_payload = json.dumps(e.encode())
                send_message(event_payload, connection)
            if self.pending_highlights:
                highlight_event = mqt.debugger.dap.messages.HighlightError(self.pending_highlights, self.source_file)
                send_message(json.dumps(highlight_event.encode()), connection)
                self.pending_highlights = []
                self._prevent_exit = True
            self.regular_checks(connection)

    def regular_checks(self, connection: socket.socket) -> None:
        """Perform regular checks and send events to the client if necessary.

        Args:
            connection (socket.socket): The client socket.
        """
        e: mqt.debugger.dap.messages.DAPEvent | None = None
        if (
            self.simulation_state.is_finished()
            and self.simulation_state.get_instruction_count() != 0
            and not self._prevent_exit
        ):
            e = mqt.debugger.dap.messages.ExitedDAPEvent(0)
            event_payload = json.dumps(e.encode())
            send_message(event_payload, connection)
        if self.can_step_back != self.simulation_state.can_step_backward():
            self.can_step_back = self.simulation_state.can_step_backward()
            e = mqt.debugger.dap.messages.CapabilitiesDAPEvent({"supportsStepBack": self.can_step_back})
            event_payload = json.dumps(e.encode())

    def handle_command(self, command: dict[str, Any]) -> tuple[dict[str, Any], mqt.debugger.dap.messages.DAPMessage]:
        """Handle an incoming command from the client and return the corresponding response.

        Args:
            command (dict[str, Any]): The command read from the client.

        Raises:
            RuntimeError: If the command is not supported.

        Returns:
            tuple[dict[str, Any], mqt.debugger.dap.messages.DAPMessage]: The response to the message as a dictionary and the message object.
        """
        if command["command"] == "setVariable":
            arguments = command.get("arguments", {})
            variables_reference = arguments.get("variablesReference")
            message_type: type[mqt.debugger.dap.messages.DAPMessage]
            message_type = (
                AmplitudeChangeDAPMessage if variables_reference == _QUANTUM_REFERENCE else BitChangeDAPMessage
            )
            message: mqt.debugger.dap.messages.DAPMessage = message_type(command)
            return (message.handle(self), message)
        for message_type in supported_messages:
            if message_type.message_type_name == command["command"]:
                msg_instance: mqt.debugger.dap.messages.DAPMessage = message_type(command)
                return (msg_instance.handle(self), msg_instance)
        msg = f"Unsupported command: {command['command']}"
        raise RuntimeError(msg)

    def handle_assertion_fail(self, connection: socket.socket) -> None:
        """Handles the sending of output events when an assertion fails.

        Args:
            connection (socket.socket): The client socket.
        """
        current_instruction = self.simulation_state.get_current_instruction()
        dependencies = self.simulation_state.get_diagnostics().get_data_dependencies(current_instruction)
        gray_out_areas: list[tuple[int, int]] = []
        for i in range(self.simulation_state.get_instruction_count()):
            if i in dependencies:
                continue
            start, end = self.simulation_state.get_instruction_position(i)
            gray_out_areas.append((start, end))

        e = mqt.debugger.dap.messages.GrayOutDAPEvent(gray_out_areas, self.source_file)
        event_payload = json.dumps(e.encode())
        send_message(event_payload, connection)

        error_causes = self.simulation_state.get_diagnostics().potential_error_causes()
        error_cause_messages = [self.format_error_cause(cause) for cause in error_causes]
        error_cause_messages = [msg for msg in error_cause_messages if msg]
        error_causes_body: str | dict[str, Any] = ""
        if not error_cause_messages:
            error_causes_body = "○ No potential error causes found"
        else:
            error_causes_body = {
                "title": f"Found {len(error_causes)} potential error cause{'s' if len(error_causes) > 1 else ''}:",
                "body": [f"({i + 1}) {msg}" for i, msg in enumerate(error_cause_messages)],
                "end": None,
            }

        (start, end) = self.simulation_state.get_instruction_position(current_instruction)
        line, column = self.code_pos_to_coordinates(start)
        instruction_code = self.source_code[start:end].replace("\r", "").replace("\n", "").strip()
        self.send_message_hierarchy(
            {
                "title": f"Assertion failed on line {line}",
                "body": [f"    {instruction_code}", "○ Highlighting dependent predecessors", error_causes_body],
            },
            line,
            column,
            connection,
            "stderr",
        )
        highlight_entries = self.collect_highlight_entries(current_instruction)
        if highlight_entries:
            highlight_event = mqt.debugger.dap.messages.HighlightError(highlight_entries, self.source_file)
            send_message(json.dumps(highlight_event.encode()), connection)
            self._prevent_exit = True

    def code_pos_to_coordinates(self, pos: int) -> tuple[int, int]:
        """Helper method to convert a code position to line and column.

        Args:
            pos (int): The 0-indexed position in the code.

        Returns:
            tuple[int, int]: The line and column, 0-or-1-indexed.
        """
        lines = self.source_code.split("\n")
        line = 1 if lines else 0
        col = 0
        for i, line_code in enumerate(lines):
            if pos <= len(line_code):
                line = i + 1
                col = pos
                break
            pos -= len(line_code) + 1
        else:
            if lines:
                line = len(lines)
                col = len(lines[-1])
        if self.columns_start_at_one:
            col += 1
        if not self.lines_start_at_one:
            line -= 1
        return (line, col)

    def code_coordinates_to_pos(self, line: int, col: int) -> int:
        """Helper method to convert a code line and column to its position idnex.

        Args:
            line (int): The 0-or-1-indexed line in the code.
            col (int): The 0-or-1-indexed column in the line.

        Returns:
            int: The 0-indexed position in the code.
        """
        lines = self.source_code.split("\n")
        if not self.lines_start_at_one:
            line += 1
        pos = 0
        for line_index in range(line - 1):
            pos += len(lines[line_index]) + 1
        pos += col
        if self.columns_start_at_one:
            pos -= 1
        return pos

    def format_error_cause(self, cause: mqt.debugger.ErrorCause) -> str:
        """Format an error cause for output.

        Args:
            cause (mqt.debugger.ErrorCause): The error cause.

        Returns:
            str: The formatted error cause.
        """
        (start_pos, _) = self.simulation_state.get_instruction_position(cause.instruction)
        start_line, _ = self.code_pos_to_coordinates(start_pos)
        return (
            "The qubits never interact with each other. Are you missing a CX gate?"
            if cause.type == mqt.debugger.ErrorCauseType.MissingInteraction
            else f"Control qubit is always zero in line {start_line}."
            if cause.type == mqt.debugger.ErrorCauseType.ControlAlwaysZero
            else ""
        )

    def collect_highlight_entries(self, failing_instruction: int) -> list[dict[str, Any]]:
        """Collect highlight entries for the current assertion failure."""
        highlights: list[dict[str, Any]] = []
        if getattr(self, "source_code", ""):
            try:
                diagnostics = self.simulation_state.get_diagnostics()
                error_causes = diagnostics.potential_error_causes()
            except RuntimeError:
                error_causes = []

            for cause in error_causes:
                message = self.format_error_cause(cause)
                reason = self._format_highlight_reason(cause.type)
                entry = self._build_highlight_entry(cause.instruction, reason, message)
                if entry is not None:
                    highlights.append(entry)

        if not highlights:
            entry = self._build_highlight_entry(
                failing_instruction,
                "assertionFailed",
                "Assertion failed at this instruction.",
            )
            if entry is not None:
                highlights.append(entry)

        return highlights

    def _build_highlight_entry(self, instruction: int, reason: str, message: str) -> dict[str, Any] | None:
        """Create a highlight entry for a specific instruction."""
        try:
            start_pos, end_pos = self.simulation_state.get_instruction_position(instruction)
        except RuntimeError:
            return None
        start_line, start_column = self.code_pos_to_coordinates(start_pos)
        end_position_exclusive = min(len(self.source_code), end_pos + 1)
        end_line, end_column = self.code_pos_to_coordinates(end_position_exclusive)
        snippet = self.source_code[start_pos : end_pos + 1].replace("\r", "")
        return {
            "instruction": int(instruction),
            "range": {
                "start": {"line": start_line, "column": start_column},
                "end": {"line": end_line, "column": end_column},
            },
            "reason": reason,
            "code": snippet.strip(),
            "message": message,
        }

    @staticmethod
    def _format_highlight_reason(cause_type: mqt.debugger.ErrorCauseType | None) -> str:
        """Return a short identifier for the highlight reason."""
        if cause_type == mqt.debugger.ErrorCauseType.MissingInteraction:
            return "missingInteraction"
        if cause_type == mqt.debugger.ErrorCauseType.ControlAlwaysZero:
            return "controlAlwaysZero"
        return "unknown"

    def queue_parse_error(self, error_message: str) -> None:
        """Store highlight data for a parse error to be emitted later."""
        line, column, detail = self._parse_error_location(error_message)
        entry = self._build_parse_error_highlight(line, column, detail)
        if entry is not None:
            self.pending_highlights = [entry]

    @staticmethod
    def _parse_error_location(error_message: str) -> tuple[int, int, str]:
        """Parse a compiler error string and extract the source location."""
        match = re.match(r"<input>:(\d+):(\d+):\s*(.*)", error_message.strip())
        if match:
            line = int(match.group(1))
            column = int(match.group(2))
            detail = match.group(3).strip()
        else:
            line = 1
            column = 1
            detail = error_message.strip()
        return (line, column, detail)

    def _build_parse_error_highlight(self, line: int, column: int, detail: str) -> dict[str, Any] | None:
        """Create a highlight entry for a parse error."""
        if not getattr(self, "source_code", ""):
            return None
        lines = self.source_code.split("\n")
        if not lines:
            return None
        line = max(1, min(line, len(lines)))
        column = max(1, column)
        line_index = line - 1
        line_text = lines[line_index]

        if column <= 1 and line_index > 0 and not line_text.strip():
            prev_index = line_index - 1
            while prev_index >= 0 and not lines[prev_index].strip():
                prev_index -= 1
            if prev_index >= 0:
                line_index = prev_index
                line = line_index + 1
                line_text = lines[line_index]
                stripped = line_text.lstrip()
                column = max(1, len(line_text) - len(stripped) + 1) if stripped else 1

        end_column = max(column, len(line_text) + 1)
        snippet = line_text.strip() or line_text
        return {
            "instruction": -1,
            "range": {
                "start": {"line": line, "column": column},
                "end": {"line": line, "column": end_column if end_column > 0 else column},
            },
            "reason": "parseError",
            "code": snippet,
            "message": detail,
        }

    def _flatten_message_parts(self, parts: list[Any]) -> list[str]:
        """Flatten nested message structures into plain text lines."""
        flattened: list[str] = []
        for part in parts:
            if isinstance(part, str):
                if part:
                    flattened.append(part)
            elif isinstance(part, dict):
                title = part.get("title")
                if isinstance(title, str) and title:
                    flattened.append(title)
                body = part.get("body")
                if isinstance(body, list):
                    flattened.extend(self._flatten_message_parts(body))
                elif isinstance(body, str) and body:
                    flattened.append(body)
                end = part.get("end")
                if isinstance(end, str) and end:
                    flattened.append(end)
            elif isinstance(part, list):
                flattened.extend(self._flatten_message_parts(part))
            elif part is not None:
                flattened.append(str(part))
        return flattened

    def send_message_hierarchy(
        self,
        message: dict[str, str | list[Any] | dict[str, Any]],
        line: int,
        column: int,
        connection: socket.socket,
        category: str = "console",
    ) -> None:
        """Send a hierarchy of messages to the client.

        Args:
            message (dict[str, str | list[str], dict[str, Any]]): An object representing the message to send. Supported keys are "title", "body", "end".
            line (int): The line number.
            column (int): The column number.
            connection (socket.socket): The client socket.
            category (str): The output category (console/stdout/stderr).
        """
        raw_body = message.get("body")
        body: list[str] | None = None
        if isinstance(raw_body, list):
            body = self._flatten_message_parts(raw_body)
        elif isinstance(raw_body, str):
            body = [raw_body]
        end_value = message.get("end")
        end = end_value if isinstance(end_value, str) else None
        title = str(message.get("title", ""))
        self.send_message_simple(title, body, end, line, column, connection, category)

    def send_message_simple(
        self,
        title: str,
        body: list[str] | None,
        end: str | None,
        line: int,
        column: int,
        connection: socket.socket,
        category: str = "console",
    ) -> None:
        """Send a simple message to the client.

        Args:
            title (str): The title of the message.
            body (list[str]): The body of the message.
            end (str | None): The end of the message.
            line (int): The line number.
            column (int): The column number.
            connection (socket.socket): The client socket.
            category (str): The output category (console/stdout/stderr).
        """
        segments: list[str] = []
        if title:
            segments.append(title)
        if body:
            segments.extend(body)
        if end:
            segments.append(end)
        if not segments:
            return
        output_text = "\n".join(segments)
        event = mqt.debugger.dap.messages.OutputDAPEvent(
            category,
            output_text,
            None,
            line,
            column,
            self.source_file,
        )
        send_message(json.dumps(event.encode()), connection)

    def send_state(self, connection: socket.socket) -> None:
        """Send the state of the current execution to the client.

        Args:
            connection (socket.socket): The client socket.
        """
        output_lines = []
        if self.simulation_state.did_assertion_fail():
            output_lines.append("Assertion failed")
        if self.simulation_state.was_breakpoint_hit():
            output_lines.append("Breakpoint hit")
        if self.simulation_state.is_finished():
            output_lines.append("Finished")
        if not output_lines:
            output_lines.append("Running")
        for line_text in output_lines:
            self.send_message_simple(line_text, None, None, 0, 0, connection)
