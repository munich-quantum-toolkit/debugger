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
#include <pybind11/pybind11.h>

namespace py = pybind11;
using namespace pybind11::literals;

PYBIND11_MODULE(MQT_DEBUGGER_MODULE_NAME, m, py::mod_gil_not_used()) {
  bindDiagnostics(m);
  bindFramework(m);
  bindBackend(m);
}
