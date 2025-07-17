# Copyright (c) 2024 - 2025 Chair for Design Automation, TUM
# Copyright (c) 2025 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

"""Represents the 'configurationDone' DAP request."""

from __future__ import annotations

from typing import TYPE_CHECKING, Any

from .dap_message import DAPMessage

if TYPE_CHECKING:
    from .. import DAPServer


class ConfigurationDoneDAPMessage(DAPMessage):
    """Represents the 'configurationDone' DAP request."""

    message_type_name: str = "configurationDone"

    def __init__(self, message: dict[str, Any]) -> None:
        """Initializes the 'ConfigurationDoneDAPMessage' instance.

        Args:
            message (dict[str, Any]): The object representing the 'ConfigurationDone' request.
        """
        super().__init__(message)

    def validate(self) -> None:
        """Validates the 'ConfigurationDoneDAPMessage' instance."""

    def handle(self, server: DAPServer) -> dict[str, Any]:
        """Performs the action requested by the 'ConfigurationDone' DAP request.

        Args:
            server (DAPServer): The DAP server that received the request.

        Returns:
            dict[str, Any]: The response to the request.
        """
        return super().handle(server)
