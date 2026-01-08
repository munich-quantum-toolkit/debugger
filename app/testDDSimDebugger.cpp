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
 * @file testDDSimDebugger.cpp
 * @brief A test application that runs the CLI frontend using the DD backend.
 */

#include "backend/dd/DDSimDebug.hpp"
#include "frontend/cli/CliFrontEnd.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using namespace mqt::debugger;

int main() {
  std::ifstream file("program.qasm");
  if (!file.is_open()) {
    file.open("../../app/code/test"
              ".qasm");
  }
  if (!file.is_open()) {
    std::cerr << "Could not open file\n";
    file.close();
    return 1;
  }

  std::stringstream buffer;
  buffer << file.rdbuf();

  const std::string code = buffer.str();

  DDSimulationState state;
  createDDSimulationState(&state);

  file.close();

  CliFrontEnd cli;
  cli.run(code.c_str(), &state.interface);

  destroyDDSimulationState(&state);

  return 0;
}
