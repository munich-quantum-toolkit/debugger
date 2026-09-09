# Copyright (c) 2024 - 2025 Chair for Design Automation, TUM Copyright (c) 2025 Munich Quantum
# Software Company GmbH All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

package_add_test(
  mqt_debugger_test
  MQT::Debugger
  utils_test.cpp
  test_compilation_statistical_slices.cpp
  test_compilation_projective_measurements.cpp
  test_utility.cpp
  test_simulation.cpp
  test_data_retrieval.cpp
  test_diagnostics.cpp
  test_custom_code.cpp
  test_parsing.cpp
  test_assertion_movement.cpp
  test_assertion_creation.cpp
  test_line_editor.cpp)

# set include directories
target_include_directories(mqt_debugger_test PUBLIC ${PROJECT_SOURCE_DIR}/test/utils)
