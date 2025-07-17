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
 * @file InterfaceBindings.hpp
 * @brief This file defines methods to be used for defining Python bindings for
 * the debugging and diagnostics backends.
 */

#pragma once

#include "pybind11/pybind11.h"

/**
 * @brief Binds the main debugging framework to Python.
 * @param m The `pybind11` module.
 */
void bindFramework(pybind11::module& m);

/**
 * @brief Binds the diagnostics backend to Python.
 * @param m The `pybind11` module.
 */
void bindDiagnostics(pybind11::module& m);
