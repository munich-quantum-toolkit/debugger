# Copyright (c) 2024 - 2025 Chair for Design Automation, TUM Copyright (c) 2025 Munich Quantum
# Software Company GmbH All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

add_mqt_debugger_binding(
  ${MQT_DEBUGGER_TARGET_NAME}-bindings
  bindings.cpp
  InterfaceBindings.cpp
  dd/DDSimDebugBindings.cpp
  MODULE_NAME
  pydebugger
  INSTALL_DIR
  .
  LINK_LIBS
  MQT::Debugger
  pybind11_json)
