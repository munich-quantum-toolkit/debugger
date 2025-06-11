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
 * @file DDSimDebugBindings.cpp
 * @brief Implements Python bindings for the DD Debugger.
 *
 * This includes bindings for creating and destroying DD SimulationStates and
 * Diagnostics states.
 */

#include "python/dd/DDSimDebugBindings.hpp"

#include "backend/dd/DDSimDebug.hpp"
#include "backend/debug.h"
#include "pybind11/pybind11.h"

namespace mqt::debugger {

void bindBackend(pybind11::module& m) {

  m.def(
      "create_ddsim_simulation_state",
      []() {
        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
        auto* state = new DDSimulationState();
        createDDSimulationState(state);
        return &state->interface;
      },
      R"(Creates a new `SimulationState` instance using the DD backend for simulation and the OpenQASM language as input format.

Returns:
    SimulationState: The created simulation state.)");

  m.def(
      "destroy_ddsim_simulation_state",
      [](SimulationState* state) {
        // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
        destroyDDSimulationState(reinterpret_cast<DDSimulationState*>(state));
        // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
      },
      R"(Delete a given DD-based `SimulationState` instance and free up resources.

Args:
    state (SimulationState): The simulation state to delete.)");
}

} // namespace mqt::debugger
