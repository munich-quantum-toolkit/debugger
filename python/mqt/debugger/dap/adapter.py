# Copyright (c) 2024 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""The main adapter module that can be run from outside the package to start the server."""

from __future__ import annotations

from .dap_server import DAPServer

if __name__ == "__main__":
    server = DAPServer()
    server.start()
