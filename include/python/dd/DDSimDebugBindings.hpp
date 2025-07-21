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
 * @file DDSimDebugBindings.hpp
 * @brief This file defines methods to be used for defining Python bindings for
 * the DD Debugger.
 */

#pragma once

#include "pybind11/pybind11.h"

/**
 * @brief Binds the dd debugging backend to Python.
 * @param m The `pybind11` module.
 */
void bindBackend(pybind11::module& m);
