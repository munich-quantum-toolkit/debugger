/*
 * Copyright (c) 2024 - 2026 Chair for Design Automation, TUM
 * Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
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

#include <nanobind/nanobind.h>

namespace nb = nanobind;
using namespace nb::literals;

// forward declarations
void bindFramework(nb::module_& m);
void bindDiagnostics(nb::module_& m);
void bindBackend(nb::module_& m);

NB_MODULE(MQT_DEBUGGER_MODULE_NAME, m) {
  bindDiagnostics(m);
  bindFramework(m);
  bindBackend(m);
}
