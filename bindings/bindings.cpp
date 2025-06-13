/*
 * Copyright (c) 2024 - 2025 Chair for Design Automation, TUM
 * Copyright (c) 2025 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

/**
 * @file bindings.cpp
 * @brief Python bindings for the debugger module.
 *
 * Central file for defining the Python bindings for the framework.
 */

#include "python/InterfaceBindings.hpp"
#include "python/dd/DDSimDebugBindings.hpp"

#include <pybind11/detail/common.h>

namespace mqt::debugger {

PYBIND11_MODULE(pydebugger, m) {
  bindDiagnostics(m);
  bindFramework(m);
  bindBackend(m);
}

} // namespace mqt::debugger
