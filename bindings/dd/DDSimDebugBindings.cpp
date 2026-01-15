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
 * @file DDSimDebugBindings.cpp
 * @brief Implements Python bindings for the DD Debugger.
 *
 * This includes bindings for creating and destroying DD SimulationStates and
 * Diagnostics states.
 */

#include "backend/dd/DDSimDebug.hpp"
#include "backend/debug.h"
#include "nanobind/nanobind.h"

namespace nb = nanobind;
using namespace nb::literals;
using namespace mqt::debugger;

// NOLINTNEXTLINE(misc-use-internal-linkage)
void bindBackend(nb::module_& m) {
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
    The created simulation state.)");

  m.def(
      "destroy_ddsim_simulation_state",
      [](SimulationState* state) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        destroyDDSimulationState(reinterpret_cast<DDSimulationState*>(state));
      },
      "state"_a,
      R"(Delete a given DD-based `SimulationState` instance and free up resources.

Args:
    state: The simulation state to delete.)");
}
