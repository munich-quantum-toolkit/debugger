# Copyright (c) 2024 - 2025 Chair for Design Automation, TUM Copyright (c) 2025 Munich Quantum
# Software Company GmbH All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

add_library(
  ${PROJECT_NAME}
  backend/dd/DDSimDebug.cpp
  backend/dd/DDSimDiagnostics.cpp
  common/ComplexMathematics.cpp
  common/ComplexMathematics.cpp
  common/parsing/AssertionParsing.cpp
  common/parsing/AssertionTools.cpp
  common/parsing/CodePreprocessing.cpp
  common/parsing/ParsingError.cpp
  common/parsing/Utils.cpp
  frontend/cli/CliFrontEnd.cpp)

# set include directories
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include
                                                  ${PROJECT_BINARY_DIR}/include)
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${Eigen3_Includes})

# link to the MQT::Core libraries
target_link_libraries(${PROJECT_NAME} PUBLIC MQT::CoreDD MQT::CoreIR MQT::CoreCircuitOptimizer)
target_link_libraries(${PROJECT_NAME} PRIVATE MQT::ProjectWarnings MQT::ProjectOptions
                                              MQT::CoreQASM)
target_link_libraries(${PROJECT_NAME} PRIVATE Eigen3::Eigen)

# add MQT alias
add_library(MQT::Debugger ALIAS ${PROJECT_NAME})
