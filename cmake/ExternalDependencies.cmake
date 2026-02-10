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
set(MQT_CORE_MINIMUM_VERSION 3.4.0
        CACHE STRING "MQT Core minimum version")
set(MQT_CORE_VERSION 3.4.1
        CACHE STRING "MQT Core version")
set(MQT_CORE_REV "8747a89766dfb943d62ed100d383cd1823d2356c"
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
  GIT_SHALLOW TRUE)
list(APPEND FETCH_PACKAGES Eigen3)
set(EIGEN_BUILD_TESTING
    OFF
    CACHE BOOL "Disable testing for Eigen")
set(BUILD_TESTING
    OFF
    CACHE BOOL "Disable general testing")
set(EIGEN_BUILD_DOC
    OFF
    CACHE BOOL "Disable documentation build for Eigen")

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

# Hide Eigen3 warnings
get_target_property(Eigen3_Includes Eigen3::Eigen INTERFACE_INCLUDE_DIRECTORIES)
