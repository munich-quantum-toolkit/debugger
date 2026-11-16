# Copyright (c) 2024 - 2026 Chair for Design Automation, TUM
# Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Tests for the DAP server."""

from __future__ import annotations

import contextlib
import json
import socket
import threading
import time
from pathlib import Path
from types import SimpleNamespace
from typing import TYPE_CHECKING, Any, cast

import pytest

from mqt.debugger.dap.dap_server import DAPServer

if TYPE_CHECKING:
    from collections.abc import Generator

RESOURCES_DIR = Path("test/python/resources/end-to-end")
BELL_QASM = str((RESOURCES_DIR / "bell.qasm").resolve())
FAIL_GHZ_QASM = str((RESOURCES_DIR / "fail_ghz.qasm").resolve())
JUMPS_QASM = str((RESOURCES_DIR.parent / "bindings" / "jumps.qasm").resolve())


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
    server.simulation_state = fake_state  # ty: ignore[invalid-assignment]

    entries = server.collect_highlight_entries(0)
    assert entries
    entry = entries[0]
    assert entry["range"]["start"]["line"] == 1
    assert entry["range"]["end"]["line"] == 1


class DAPClient:
    """DAP test client that communicates with a DAPServer over a socket."""

    def __init__(self, host: str, port: int) -> None:
        """Create a client connected to the given host and port."""
        self.sock = socket.create_connection((host, port), timeout=5.0)
        self.seq = 1
        self._buffer = b""
        self.events: list[dict[str, Any]] = []
        self.disconnected = False

    def send_request(self, command: str, arguments: dict[str, Any] | None = None) -> int:
        """Send a DAP request message and return its sequence number."""
        req_seq = self.seq
        self.seq += 1
        payload = {
            "seq": req_seq,
            "type": "request",
            "command": command,
            "arguments": arguments or {},
        }
        msg = json.dumps(payload).replace("\n", "\r\n")
        header = f"Content-Length: {len(msg)}\r\n\r\n".encode("ascii")
        self.sock.sendall(header + msg.encode("utf-8"))
        return req_seq

    def receive_message(self, timeout: float = 5.0) -> dict[str, Any]:
        """Read a single DAP message from the socket."""
        self.sock.settimeout(timeout)
        while True:
            idx = self._buffer.find(b"\r\n\r\n")
            if idx != -1:
                header = self._buffer[:idx].decode("ascii", errors="replace")
                content_length = None
                for line in header.split("\r\n"):
                    if line.lower().startswith("content-length:"):
                        content_length = int(line.split(":")[1].strip())
                        break
                if content_length is not None:
                    body_start = idx + 4
                    if len(self._buffer) >= body_start + content_length:
                        body = self._buffer[body_start : body_start + content_length]
                        self._buffer = self._buffer[body_start + content_length :]
                        return cast("dict[str, Any]", json.loads(body.decode("utf-8")))
            chunk = self.sock.recv(4096)
            if not chunk:
                msg = "Connection closed while waiting for DAP message."
                raise EOFError(msg)
            self._buffer += chunk

    def receive_response(self, command: str, timeout: float = 5.0) -> dict[str, Any]:
        """Wait for the response to a specific command, queueing any events."""
        start = time.time()
        while time.time() - start < timeout:
            msg = self.receive_message(timeout=timeout)
            if msg.get("type") == "response" and msg.get("command") == command:
                if command == "disconnect":
                    self.disconnected = True
                return msg
            if msg.get("type") == "event":
                self.events.append(msg)
        msg_err = f"Timed out waiting for response to {command}."
        raise TimeoutError(msg_err)

    def wait_for_event(self, event_name: str, timeout: float = 5.0) -> dict[str, Any]:
        """Wait for a specific DAP event by name."""
        for i, ev in enumerate(self.events):
            if ev.get("event") == event_name:
                return self.events.pop(i)
        start = time.time()
        while time.time() - start < timeout:
            msg = self.receive_message(timeout=timeout)
            if msg.get("type") == "event":
                if msg.get("event") == event_name:
                    return msg
                self.events.append(msg)
        msg_err = f"Timed out waiting for event {event_name}."
        raise TimeoutError(msg_err)

    def close(self) -> None:
        """Close the socket connection cleanly."""
        with contextlib.suppress(OSError):
            self.sock.shutdown(socket.SHUT_WR)
        self.sock.close()


@pytest.fixture
def dap_session() -> Generator[tuple[DAPServer, DAPClient], None, None]:
    """Start a DAP server in a background thread and yield a connected client."""
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind(("127.0.0.1", 0))
        port = s.getsockname()[1]

    server = DAPServer(host="127.0.0.1", port=port)
    server_thread = threading.Thread(target=server.start, daemon=True)
    server_thread.start()

    client: DAPClient | None = None
    for _ in range(50):
        try:
            client = DAPClient("127.0.0.1", port)
            break
        except OSError:
            time.sleep(0.05)
    assert client is not None, "Failed to connect to DAP server."

    try:
        yield server, client
    finally:
        if not client.disconnected:
            with contextlib.suppress(OSError, TimeoutError, EOFError):
                client.send_request("disconnect")
                client.receive_response("disconnect", timeout=1.0)
        client.close()
        server_thread.join(timeout=2.0)


@pytest.fixture
def launched_session(
    dap_session: tuple[DAPServer, DAPClient],
) -> tuple[DAPServer, DAPClient]:
    """Initialize and launch bell.qasm with stopOnEntry enabled."""
    server, client = dap_session
    client.send_request("initialize", {"adapterID": "mqtqasm"})
    init_resp = client.receive_response("initialize")
    assert init_resp["success"] is True

    client.send_request("launch", {"program": BELL_QASM, "stopOnEntry": True})
    launch_resp = client.receive_response("launch")
    assert launch_resp["success"] is True
    client.wait_for_event("initialized")
    client.wait_for_event("grayOut")
    stopped = client.wait_for_event("stopped")
    assert stopped["body"]["reason"] == "entry"

    client.send_request("configurationDone")
    config_resp = client.receive_response("configurationDone")
    assert config_resp["success"] is True

    return server, client


def test_initialize_success(dap_session: tuple[DAPServer, DAPClient]) -> None:
    """Check that initialize returns capabilities."""
    _, client = dap_session
    client.send_request("initialize", {"adapterID": "mqtqasm"})
    resp = client.receive_response("initialize")
    assert resp["success"] is True
    assert resp["command"] == "initialize"
    body = resp["body"]
    assert body["supportsConfigurationDoneRequest"] is True
    assert body["supportsStepBack"] is True
    assert body["supportsSetVariable"] is True
    assert body["supportsRestartFrame"] is True
    assert body["supportsTerminateRequest"] is True
    assert body["supportsRestartRequest"] is True
    assert body["supportsExceptionInfoRequest"] is True


def test_initialize_invalid_adapter(dap_session: tuple[DAPServer, DAPClient]) -> None:
    """Ensure initialization fails with an unsupported adapter ID."""
    _, client = dap_session
    client.send_request("initialize", {"adapterID": "other_adapter"})
    resp = client.receive_response("initialize")
    assert resp["success"] is False
    assert "Adapter ID must be `mqtqasm`" in resp["message"]


def test_launch_missing_program(dap_session: tuple[DAPServer, DAPClient]) -> None:
    """Ensure launch fails when the requested file does not exist."""
    _, client = dap_session
    client.send_request("initialize", {"adapterID": "mqtqasm"})
    client.receive_response("initialize")

    client.send_request("launch", {"program": "/nonexistent/file.qasm"})
    resp = client.receive_response("launch")
    assert resp["success"] is False
    assert "does not exist" in resp["message"]


def test_unsupported_command(dap_session: tuple[DAPServer, DAPClient]) -> None:
    """Ensure server returns an error response for unknown DAP commands."""
    _, client = dap_session
    client.send_request("fooBar")
    resp = client.receive_response("fooBar")
    assert resp["success"] is False
    assert "Unsupported command" in resp["message"]


def test_launch_and_configuration_done(launched_session: tuple[DAPServer, DAPClient]) -> None:
    """Verify launch and configuration sequence."""
    server, _ = launched_session
    assert server.source_file["name"] == "bell.qasm"
    assert len(server.source_code) > 0


def test_threads_and_stack_trace(launched_session: tuple[DAPServer, DAPClient]) -> None:
    """Check threads and stack trace information."""
    _, client = launched_session
    client.send_request("threads")
    threads_resp = client.receive_response("threads")
    assert threads_resp["success"] is True
    threads = threads_resp["body"]["threads"]
    assert len(threads) == 1
    assert threads[0]["id"] == 1
    assert threads[0]["name"] == "Main Thread"

    client.send_request("stackTrace", {"threadId": 1})
    stack_resp = client.receive_response("stackTrace")
    assert stack_resp["success"] is True
    frames = stack_resp["body"]["stackFrames"]
    assert len(frames) >= 1
    assert frames[0]["name"] == "main"
    assert frames[0]["line"] == 1
    assert frames[0]["source"]["name"] == "bell.qasm"


def test_scopes_and_variables(launched_session: tuple[DAPServer, DAPClient]) -> None:
    """Inspect classical registers and quantum state variables."""
    _, client = launched_session
    client.send_request("scopes", {"frameId": 0})
    scopes_resp = client.receive_response("scopes")
    assert scopes_resp["success"] is True
    scopes = scopes_resp["body"]["scopes"]
    assert len(scopes) == 2
    assert scopes[0]["name"] == "Classical Registers"
    assert scopes[1]["name"] == "Quantum State"

    classical_ref = scopes[0]["variablesReference"]
    quantum_ref = scopes[1]["variablesReference"]

    client.send_request("variables", {"variablesReference": classical_ref})
    classical_vars_resp = client.receive_response("variables")
    assert classical_vars_resp["success"] is True
    assert isinstance(classical_vars_resp["body"]["variables"], list)

    client.send_request("variables", {"variablesReference": quantum_ref})
    quantum_vars_resp = client.receive_response("variables")
    assert quantum_vars_resp["success"] is True
    q_vars = quantum_vars_resp["body"]["variables"]
    # 2 qubits produce 4 amplitudes: |00>, |01>, |10>, |11>
    assert len(q_vars) == 4
    names = [v["name"] for v in q_vars]
    assert names == ["|00>", "|01>", "|10>", "|11>"]
    # Initially in state |00>
    assert "1.000000" in q_vars[0]["value"]


def test_stepping_and_step_back(launched_session: tuple[DAPServer, DAPClient]) -> None:
    """Step forward and backward through instructions."""
    _, client = launched_session
    client.send_request("next", {"threadId": 1})
    next_resp = client.receive_response("next")
    assert next_resp["success"] is True
    event = client.wait_for_event("stopped")
    assert event["body"]["reason"] == "step"

    client.send_request("stackTrace", {"threadId": 1})
    stack_resp = client.receive_response("stackTrace")
    new_line = stack_resp["body"]["stackFrames"][0]["line"]
    assert new_line > 1

    client.send_request("stepBack", {"threadId": 1})
    step_back_resp = client.receive_response("stepBack")
    assert step_back_resp["success"] is True
    back_event = client.wait_for_event("stopped")
    assert back_event["body"]["reason"] == "step"


def test_step_in_and_step_out(launched_session: tuple[DAPServer, DAPClient]) -> None:
    """Test step-in and step-out commands."""
    _, client = launched_session
    client.send_request("stepIn", {"threadId": 1})
    in_resp = client.receive_response("stepIn")
    assert in_resp["success"] is True
    in_event = client.wait_for_event("stopped")
    assert in_event["body"]["reason"] == "step"

    client.send_request("stepOut", {"threadId": 1})
    out_resp = client.receive_response("stepOut")
    assert out_resp["success"] is True
    out_event = client.wait_for_event("stopped")
    assert out_event["body"]["reason"] == "step"


def test_breakpoints_and_continue(launched_session: tuple[DAPServer, DAPClient]) -> None:
    """Set a breakpoint on a gate and continue until it is hit."""
    _, client = launched_session
    # Line 4 of bell.qasm is: cx q[0], q[1];
    client.send_request(
        "setBreakpoints",
        {
            "source": {"name": "bell.qasm", "path": BELL_QASM},
            "breakpoints": [{"line": 4, "column": 1}],
        },
    )
    bpt_resp = client.receive_response("setBreakpoints")
    assert bpt_resp["success"] is True
    bpts = bpt_resp["body"]["breakpoints"]
    assert len(bpts) == 1
    assert bpts[0]["verified"] is True

    client.send_request("continue", {"threadId": 1})
    cont_resp = client.receive_response("continue")
    assert cont_resp["success"] is True
    stop_event = client.wait_for_event("stopped")
    assert stop_event["body"]["reason"] in {"instruction breakpoint", "breakpoint_instruction", "step"}


def test_set_breakpoints_wrong_file(launched_session: tuple[DAPServer, DAPClient]) -> None:
    """Breakpoints in non-active files return unverified status."""
    _, client = launched_session
    client.send_request(
        "setBreakpoints",
        {
            "source": {"name": "other.qasm", "path": "/fake/other.qasm"},
            "breakpoints": [{"line": 1}],
        },
    )
    resp = client.receive_response("setBreakpoints")
    assert resp["success"] is True
    bpts = resp["body"]["breakpoints"]
    assert len(bpts) == 1
    assert bpts[0]["verified"] is False


def test_set_variable_amplitude(launched_session: tuple[DAPServer, DAPClient]) -> None:
    """Modify quantum state amplitudes using setVariable."""
    _, client = launched_session
    client.send_request("next", {"threadId": 1})
    client.receive_response("next")
    client.wait_for_event("stopped")

    client.send_request("next", {"threadId": 1})
    client.receive_response("next")
    client.wait_for_event("stopped")

    client.send_request(
        "setVariable",
        {
            "variablesReference": 2,
            "name": "|00>",
            "value": "0.0 + 1.0i",
        },
    )
    resp = client.receive_response("setVariable")
    assert resp["success"] is True
    assert "1i" in resp["body"]["value"]

    client.send_request("variables", {"variablesReference": 2})
    vars_resp = client.receive_response("variables")
    matching = [v for v in vars_resp["body"]["variables"] if v["name"] == "|00>"]
    assert len(matching) == 1
    assert "1.000000i" in matching[0]["value"]


def test_set_variable_bit(dap_session: tuple[DAPServer, DAPClient], tmp_path: Path) -> None:
    """Modify classical bit variables using setVariable."""
    _, client = dap_session
    client.send_request("initialize", {"adapterID": "mqtqasm"})
    client.receive_response("initialize")

    # Create a circuit with a classical register
    qasm_file = tmp_path / "classical.qasm"
    qasm_file.write_text("qreg q[1];\ncreg c[1];\nmeasure q[0] -> c[0];\n")

    client.send_request("launch", {"program": str(qasm_file), "stopOnEntry": True})
    client.receive_response("launch")
    client.wait_for_event("initialized")
    client.wait_for_event("grayOut")
    client.wait_for_event("stopped")

    client.send_request("configurationDone")
    client.receive_response("configurationDone")

    client.send_request(
        "setVariable",
        {
            "variablesReference": 1,
            "name": "c[0]",
            "value": "1",
        },
    )
    resp = client.receive_response("setVariable")
    assert resp["success"] is True
    assert resp["body"]["value"] in {"1", "True", True}


def test_reverse_continue(launched_session: tuple[DAPServer, DAPClient]) -> None:
    """Run simulation backward with reverseContinue."""
    _, client = launched_session
    client.send_request("next", {"threadId": 1})
    client.receive_response("next")
    client.wait_for_event("stopped")

    client.send_request("reverseContinue", {"threadId": 1})
    rev_resp = client.receive_response("reverseContinue")
    assert rev_resp["success"] is True
    event = client.wait_for_event("stopped")
    assert event["body"]["reason"] in {"step", "entry"}


def test_restart(launched_session: tuple[DAPServer, DAPClient]) -> None:
    """Restart execution from the beginning."""
    _, client = launched_session
    client.send_request("next", {"threadId": 1})
    client.receive_response("next")
    client.wait_for_event("stopped")

    client.send_request("restart", {"arguments": {"program": BELL_QASM, "stopOnEntry": True}})
    resp = client.receive_response("restart")
    assert resp["success"] is True
    client.wait_for_event("grayOut")
    stopped = client.wait_for_event("stopped")
    assert stopped["body"]["reason"] == "entry"


def test_restart_frame(launched_session: tuple[DAPServer, DAPClient]) -> None:
    """Restart the current stack frame."""
    _, client = launched_session
    client.send_request("next", {"threadId": 1})
    client.receive_response("next")
    client.wait_for_event("stopped")

    client.send_request("restartFrame", {"frameId": 1})
    resp = client.receive_response("restartFrame")
    assert resp["success"] is True
    stopped = client.wait_for_event("stopped")
    assert stopped["body"]["reason"] == "step"


def test_restart_frame_nested(dap_session: tuple[DAPServer, DAPClient]) -> None:
    """Restart a nested stack frame."""
    _, client = dap_session
    client.send_request("initialize", {"adapterID": "mqtqasm"})
    client.receive_response("initialize")

    client.send_request("launch", {"program": JUMPS_QASM, "stopOnEntry": True})
    client.receive_response("launch")
    client.wait_for_event("initialized")
    client.wait_for_event("grayOut")
    client.wait_for_event("stopped")

    # Set breakpoint at line 24 (call to gate create_ghz)
    client.send_request(
        "setBreakpoints",
        {
            "source": {"name": "jumps.qasm", "path": JUMPS_QASM},
            "breakpoints": [{"line": 24, "column": 1}],
        },
    )
    bpt_resp = client.receive_response("setBreakpoints")
    assert bpt_resp["success"] is True

    client.send_request("configurationDone")
    client.receive_response("configurationDone")

    client.send_request("continue", {"threadId": 1})
    client.receive_response("continue")
    stop_event = client.wait_for_event("stopped")
    assert stop_event["body"]["reason"] in {"instruction breakpoint", "breakpoint_instruction", "step"}

    # Step in to enter create_ghz subroutine (stack depth 2)
    client.send_request("stepIn", {"threadId": 1})
    step_resp = client.receive_response("stepIn")
    assert step_resp["success"] is True
    client.wait_for_event("stopped")

    # Verify stack depth >= 2 via stackTrace
    client.send_request("stackTrace", {"threadId": 1})
    st_resp = client.receive_response("stackTrace")
    assert st_resp["success"] is True
    assert len(st_resp["body"]["stackFrames"]) >= 2

    # Restart frame 2 (the nested frame)
    client.send_request("restartFrame", {"frameId": 2})
    resp = client.receive_response("restartFrame")
    assert resp["success"] is True
    stopped = client.wait_for_event("stopped")
    assert stopped["body"]["reason"] == "step"


def test_pause(launched_session: tuple[DAPServer, DAPClient]) -> None:
    """Send pause request during execution."""
    _, client = launched_session
    client.send_request("pause", {"threadId": 1})
    resp = client.receive_response("pause")
    assert resp["success"] is True
    stopped = client.wait_for_event("stopped")
    assert stopped["body"]["reason"] == "pause"


def test_set_exception_breakpoints(launched_session: tuple[DAPServer, DAPClient]) -> None:
    """Configure exception breakpoint filters."""
    _, client = launched_session
    client.send_request("setExceptionBreakpoints", {"filters": ["assert-ent"]})
    resp = client.receive_response("setExceptionBreakpoints")
    assert resp["success"] is True
    assert len(resp["body"]["breakpoints"]) == 1
    assert resp["body"]["breakpoints"][0]["verified"] is True


def test_assertion_failure_and_diagnostics(dap_session: tuple[DAPServer, DAPClient]) -> None:
    """Detect assertion failure and retrieve exception details."""
    _, client = dap_session
    client.send_request("initialize", {"adapterID": "mqtqasm"})
    client.receive_response("initialize")

    # Run fail_ghz.qasm without stopOnEntry so it halts on the failed assertion
    client.send_request("launch", {"program": FAIL_GHZ_QASM, "stopOnEntry": False})
    launch_resp = client.receive_response("launch")
    assert launch_resp["success"] is True

    stopped = client.wait_for_event("stopped")
    assert stopped["body"]["reason"] == "exception"
    assert "assertion" in stopped["body"]["description"].lower()

    client.send_request("exceptionInfo", {"threadId": 1})
    exc_resp = client.receive_response("exceptionInfo")
    assert exc_resp["success"] is True
    assert "assert-ent" in exc_resp["body"]["exceptionId"]
    assert len(exc_resp["body"]["description"]) > 0


def test_terminate(launched_session: tuple[DAPServer, DAPClient]) -> None:
    """Terminate the debugging session."""
    _, client = launched_session
    client.send_request("terminate")
    resp = client.receive_response("terminate")
    assert resp["success"] is True
    client.wait_for_event("terminated")
    exited = client.wait_for_event("exited")
    assert exited["body"]["exitCode"] == 143
    client.disconnected = True
