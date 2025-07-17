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
import socket
import sys
from typing import Any, cast

import mqt.debugger

from .messages import (
    ConfigurationDoneDAPMessage,
    ContinueDAPMessage,
    DisconnectDAPMessage,
    ExceptionInfoDAPMessage,
    InitializeDAPMessage,
    LaunchDAPMessage,
    NextDAPMessage,
    PauseDAPMessage,
    Request,
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
            self.regular_checks(connection)

    def regular_checks(self, connection: socket.socket) -> None:
        """Perform regular checks and send events to the client if necessary.

        Args:
            connection (socket.socket): The client socket.
        """
        e: mqt.debugger.dap.messages.DAPEvent | None = None
        if self.simulation_state.is_finished() and self.simulation_state.get_instruction_count() != 0:
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
        for message_type in supported_messages:
            if message_type.message_type_name == command["command"]:
                message = message_type(command)
                return (message.handle(self), message)
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
        )

    def code_pos_to_coordinates(self, pos: int) -> tuple[int, int]:
        """Helper method to convert a code position to line and column.

        Args:
            pos (int): The 0-indexed position in the code.

        Returns:
            tuple[int, int]: The line and column, 0-or-1-indexed.
        """
        lines = self.source_code.split("\n")
        line = 0
        col = 0
        for i, line_code in enumerate(lines):
            if pos < len(line_code):
                line = i + 1
                col = pos
                break
            pos -= len(line_code) + 1
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

    def send_message_hierarchy(
        self, message: dict[str, str | list[Any] | dict[str, Any]], line: int, column: int, connection: socket.socket
    ) -> None:
        """Send a hierarchy of messages to the client.

        Args:
            message (dict[str, str | list[str], dict[str, Any]]): An object representing the message to send. Supported keys are "title", "body", "end".
            line (int): The line number.
            column (int): The column number.
            connection (socket.socket): The client socket.
        """
        if "title" in message:
            title_event = mqt.debugger.dap.messages.OutputDAPEvent(
                "console", cast("str", message["title"]), "start", line, column, self.source_file
            )
            send_message(json.dumps(title_event.encode()), connection)

        if "body" in message:
            body = message["body"]
            if isinstance(body, list):
                for msg in body:
                    if isinstance(msg, dict):
                        self.send_message_hierarchy(msg, line, column, connection)
                    else:
                        output_event = mqt.debugger.dap.messages.OutputDAPEvent(
                            "console", msg, None, line, column, self.source_file
                        )
                        send_message(json.dumps(output_event.encode()), connection)
            elif isinstance(body, dict):
                self.send_message_hierarchy(body, line, column, connection)
            elif isinstance(body, str):
                output_event = mqt.debugger.dap.messages.OutputDAPEvent(
                    "console", body, None, line, column, self.source_file
                )
                send_message(json.dumps(output_event.encode()), connection)

        if "end" in message or "title" in message:
            end_event = mqt.debugger.dap.messages.OutputDAPEvent(
                "console", cast("str", message.get("end")), "end", line, column, self.source_file
            )
            send_message(json.dumps(end_event.encode()), connection)
