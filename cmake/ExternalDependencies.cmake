# Copyright (c) 2024 - 2026 Chair for Design Automation, TUM
# Copyright (c) 2025 - 2026 Munich Quantum Software Company GmbH
# All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

include(FetchContent)
set(FETCH_PACKAGES "")

if(BUILD_MQT_DEBUGGER_BINDINGS)
  execute_process(
    COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
    OUTPUT_STRIP_TRAILING_WHITESPACE
    OUTPUT_VARIABLE nanobind_ROOT)
  find_package(nanobind CONFIG REQUIRED)
endif()

# ---------------------------------------------------------------------------------Fetch MQT Core
# cmake-format: off
set(MQT_CORE_MINIMUM_VERSION 3.6.0
        CACHE STRING "MQT Core minimum version")
set(MQT_CORE_VERSION 3.6.1
        CACHE STRING "MQT Core version")
set(MQT_CORE_REV "8224856776df527ff2e9911fb1751572fefaf80a"
        CACHE STRING "MQT Core identifier (tag, branch or commit hash)")
set(MQT_CORE_REPO_OWNER "munich-quantum-toolkit"
        CACHE STRING "MQT Core repository owner (change when using a fork)")
# cmake-format: on
FetchContent_Declare(
  mqt-core
  GIT_REPOSITORY https://github.com/${MQT_CORE_REPO_OWNER}/core.git
  GIT_TAG ${MQT_CORE_REV})
list(APPEND FETCH_PACKAGES mqt-core)

# ---------------------------------------------------------------------------------Fetch Eigen3
# cmake-format: off
set(EIGEN_VERSION 3.4.0
        CACHE STRING "Eigen3 version")
# cmake-format: on
FetchContent_Declare(
  Eigen3
  GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
  GIT_TAG ${EIGEN_VERSION}
  GIT_SHALLOW TRUE
  # null is a non-existent subdirectory in Eigen3's repository, so add_subdirectory() becomes a
  # no-op. This stops Eigen3's own CMakeLists.txt from running, which is only relevant for its
  # documentation and tests.
  SOURCE_SUBDIR null)
list(APPEND FETCH_PACKAGES Eigen3)

if(BUILD_MQT_DEBUGGER_TESTS)
  set(gtest_force_shared_crt
      ON
      CACHE BOOL "" FORCE)
  set(GTEST_VERSION
      1.17.0
      CACHE STRING "Google Test version")
  set(GTEST_URL https://github.com/google/googletest/archive/refs/tags/v${GTEST_VERSION}.tar.gz)
  FetchContent_Declare(googletest URL ${GTEST_URL} FIND_PACKAGE_ARGS ${GTEST_VERSION} NAMES GTest)
  list(APPEND FETCH_PACKAGES googletest)
endif()

# Make all declared dependencies available.
FetchContent_MakeAvailable(${FETCH_PACKAGES})

add_library(Eigen3::Eigen INTERFACE IMPORTED)
set_target_properties(Eigen3::Eigen PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${eigen3_SOURCE_DIR}")

# Hide Eigen3 warnings
get_target_property(Eigen3_Includes Eigen3::Eigen INTERFACE_INCLUDE_DIRECTORIES)
