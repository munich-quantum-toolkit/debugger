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
 * @file test_simulation.cpp
 * @brief Test the functionality of the simulation methods.
 */

#include "backend/dd/DDSimDebug.hpp"
#include "backend/debug.h"
#include "common.h"
#include "utils_test.hpp"

#include <cstddef>
#include <gtest/gtest.h>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace mqt::debugger::test {

/**
 * @brief Fixture for testing the correctness of the simulation methods.
 *
 * This parametrized fixture creates a DDSimulationState. The parameter value
 * defines the specific file to run the tests on.
 */
class SimulationTest : public testing::TestWithParam<std::string> {
  void SetUp() override {
    createDDSimulationState(&ddState);
    state = &ddState.interface;
    loadFromFile(GetParam());
  }

protected:
  /**
   * @brief The DDSimulationState to use for testing.
   */
  DDSimulationState ddState{};
  /**
   * @brief A reference to the SimulationState interface for easier access.
   */
  SimulationState* state = nullptr;

  /**
   * @brief Load the code from the file with the given name.
   *
   * The given file should be located in the `circuits` directory and use the
   * `.qasm` extension.
   * @param testName The name of the file to load (not including the `circuits`
   * directory path and the extension).
   */
  void loadFromFile(const std::string& testName) {
    const auto code = readFromCircuitsPath(testName);
    const auto result = state->loadCode(state, code.c_str());
    ASSERT_EQ(result.status, OK);
  }

  /**
   * @brief Move the simulation through the given sequence of movements and
   * check the results.
   *
   * The caller provides a list of actions and the expected instruction after
   * each action. The method will execute the actions and check if the expected
   * instruction is reached. It will also test that no error is returned by any
   * step.\n\n
   *
   * By providing the movement type `assertion`, the method will check if the
   * previous step failed an assertion.\n\n
   *
   * Supported movement types are:\n
   * - `sf`: step forward\n
   * - `of`: step over forward\n
   * - `uf`: step out forward\n
   * - `rf`: run simulation forward\n
   * - `sb`: step backward\n
   * - `ob`: step over backward\n
   * - `ub`: step out backward\n
   * - `rb`: run simulation backward\n
   * - `assertion`: check if the previous step failed an assertion
   *
   * @param movements A list of pairs representing the desired movement. The
   * first element of the pair is the movement type, the second element is the
   * expected instruction after the movement.
   */
  void moveAndCheck(
      const std::vector<std::pair<const std::string, size_t>>& movements) {
    size_t step = 0;
    for (const auto& movement : movements) {
      Result (*movementFunction)(SimulationState*) = nullptr;
      if (movement.first == "sf") {
        movementFunction = state->stepForward;
      } else if (movement.first == "of") {
        movementFunction = state->stepOverForward;
      } else if (movement.first == "uf") {
        movementFunction = state->stepOutForward;
      } else if (movement.first == "rf") {
        movementFunction = state->runSimulation;
      } else if (movement.first == "sb") {
        movementFunction = state->stepBackward;
      } else if (movement.first == "ob") {
        movementFunction = state->stepOverBackward;
      } else if (movement.first == "ub") {
        movementFunction = state->stepOutBackward;
      } else if (movement.first == "rb") {
        movementFunction = state->runSimulationBackward;
      } else if (movement.first == "assertion") {
        ASSERT_TRUE(state->didAssertionFail(state))
            << "Expected assertion to fail at step " << step << " in "
            << GetParam() << "\n";
        step++;
        continue;
      } else {
        FAIL() << "Unknown movement type " << movement.first << "\n";
      }
      ASSERT_EQ(movementFunction(state), Result::OK)
          << "Movement " << movement.first << " failed at step " << step
          << " in " << GetParam() << "\n";
      ASSERT_EQ(state->getCurrentInstruction(state), movement.second)
          << "Movement " << movement.first
          << " did not reach expected instruction " << movement.second
          << " at step " << step << " in " << GetParam() << "\n";
      step++;
    }
  }
};

/**
 * @test Repeatedly step forward through the code until the end is reached and
 * check, whether the expected instructions are traversed.
 *
 * For `failing-assertions`, this is a straight line from 0 to 10, repeating the
 * indices of all failed assertions.\n For `complex-jumps`, this includes
 * several calls leading to jumps through the code.
 */
TEST_P(SimulationTest, StepThroughCode) {
  const std::map<const std::string, std::vector<size_t>> expected = {
      {"complex-jumps", {0, 1,  4, 9,  12, 5, 6, 10, 2,  3,  11, 7, 10, 2,
                         3, 11, 8, 13, 10, 2, 3, 11, 14, 10, 2,  3, 11}},
      {"failing-assertions", {0, 1, 2, 3, 4, 5, 6, 6, 7, 8, 9, 9, 10}}};
  for (const auto exp : expected.at(GetParam())) {
    ASSERT_EQ(state->getCurrentInstruction(state), exp);
    state->stepForward(state);
  }
}

/**
 * @test Test the correctness of the `getStackDepth` and `getStackTrace` methods
 * of the debugging interface at each instruction.
 *
 * For `failing-assertions`, the stack trace is always 1, as there are no jumps
 * or calls.\n For `complex-jumps`, the stack traces are different, as it
 * includes multiple calls and jumps.
 */
TEST_P(SimulationTest, StackTraceRetrieval) {
  const std::map<const std::string, std::vector<size_t>> expectedDepths = {
      {"complex-jumps", {1, 1, 1, 1, 1, 2, 2, 3, 4, 4, 3, 2, 3, 4,
                         4, 3, 2, 1, 2, 3, 3, 2, 1, 2, 3, 3, 2}},
      {"failing-assertions", {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}}};
  const std::map<const std::string, std::vector<std::vector<size_t>>>
      expectedStacks = {
          {"complex-jumps",
           {{0},
            {1},
            {4},
            {9},
            {12},
            {5, 12},
            {6, 12},
            {10, 6, 12},
            {2, 10, 6, 12},
            {3, 10, 6, 12},
            {11, 6, 12},
            {7, 12},
            {10, 7, 12},
            {2, 10, 7, 12},
            {3, 10, 7, 12},
            {11, 7, 12},
            {8, 12},
            {13},
            {10, 13},
            {2, 10, 13},
            {3, 10, 13},
            {11, 13},
            {14},
            {10, 14},
            {2, 10, 14},
            {3, 10, 14},
            {11, 14}}},
          {"failing-assertions",
           {{0}, {1}, {2}, {3}, {4}, {5}, {6}, {6}, {7}, {8}, {9}, {9}, {10}}}};

  for (size_t index = 0; index < expectedDepths.at(GetParam()).size();
       index++) {
    const auto exp = expectedDepths.at(GetParam())[index];
    size_t depth = 0;
    ASSERT_EQ(state->getStackDepth(state, &depth), Result::OK);
    ASSERT_EQ(depth, exp) << "Depth computation failed for instruction "
                          << state->getCurrentInstruction(state) << " at index "
                          << index << " in " << GetParam() << "\n";
    for (size_t depthToTest = 1; depthToTest <= depth + 1; depthToTest++) {
      std::vector<size_t> stack(depthToTest);
      ASSERT_EQ(state->getStackTrace(state, depthToTest, stack.data()),
                Result::OK)
          << "Failed to get stack trace for depth " << depthToTest
          << " for instruction " << state->getCurrentInstruction(state)
          << " in " << GetParam() << "\n";
      const std::vector<size_t>& expectedStack =
          expectedStacks.at(GetParam()).at(index);
      for (size_t i = 0; i < (depthToTest > depth ? depth : depthToTest); i++) {
        ASSERT_EQ(stack[i], expectedStack[i])
            << "Failed for index " << i << " at depth " << depthToTest
            << " for instruction " << state->getCurrentInstruction(state)
            << " in " << GetParam() << "\n";
      }
    }

    state->stepForward(state);
  }
}

/**
 * @test Test that breakpoints can be set correctly at top-level instructions in
 * the code.
 *
 * This tests that the breakpoints land at the expected instructions and that a
 * running simulation will pause at the correct instruction.\n It also tests
 * that removing breakpoints works as expected.
 */
TEST_P(SimulationTest, TopLevelBreakpoints) {
  const std::map<const std::string, std::vector<size_t>> breakpointPositions = {
      {"complex-jumps", {174, 451, 488, 525}},
      {"failing-assertions", {58, 322, 374, 427, 487}}};
  const std::map<const std::string, std::vector<size_t>>
      expectedBreakpointPositions = {{"complex-jumps", {4, 12, 13, 14}},
                                     {"failing-assertions", {1, 5, 6, 7, 8}}};

  for (size_t index = 0; index < breakpointPositions.at(GetParam()).size();
       index++) {
    const auto breakpoint = breakpointPositions.at(GetParam())[index];
    size_t targetInstruction = 0;
    ASSERT_EQ(state->setBreakpoint(state, breakpoint, &targetInstruction),
              Result::OK)
        << "Failed to set breakpoint at instruction " << breakpoint << " in "
        << GetParam() << "\n";
    ASSERT_EQ(targetInstruction,
              expectedBreakpointPositions.at(GetParam())[index])
        << "Breakpoint set at wrong instruction for breakpoint " << breakpoint
        << " in " << GetParam() << "\n";
  }

  for (const auto instruction : expectedBreakpointPositions.at(GetParam())) {
    ASSERT_EQ(state->runSimulation(state), Result::OK)
        << "Failed to run simulation in " << GetParam() << "\n";
    while (state->didAssertionFail(state)) {
      ASSERT_EQ(state->runSimulation(state), Result::OK)
          << "Failed to run simulation in " << GetParam() << "\n";
    }
    ASSERT_EQ(state->getCurrentInstruction(state), instruction)
        << "Breakpoint not hit at expected instruction " << instruction
        << " in " << GetParam() << "\n";
    ASSERT_TRUE(state->wasBreakpointHit(state));
  }

  ASSERT_EQ(state->runSimulationBackward(state), Result::OK);
  ASSERT_TRUE(state->wasBreakpointHit(state));

  ASSERT_EQ(state->clearBreakpoints(state), Result::OK)
      << "Failed to clear breakpoints in " << GetParam() << "\n";
  ASSERT_EQ(state->runSimulationBackward(state), Result::OK);
  ASSERT_FALSE(state->wasBreakpointHit(state))
      << "Breakpoint hit after clearing in " << GetParam() << "\n";
  while (!state->isFinished(state)) {
    ASSERT_EQ(state->runSimulation(state), Result::OK);
    ASSERT_FALSE(state->wasBreakpointHit(state))
        << "Breakpoint hit after clearing in " << GetParam() << "\n";
  }
}

/**
 * @test Tests the correctness of the `pauseSimulation` method.
 *
 * The protocol allows us to be lenient with the 'pause' operation. We do not
 * need to pause immediately, instead we can pause at the next convenient
 * time. 'stepOver' and 'stepOut' methods therefore do not necessarily stop if
 * 'pause' was called before they started. `run` (in both directions), on the
 * other hand, will ALWAYS pause immediately, even if 'pause' was called
 * before they started.
 */
TEST_P(SimulationTest, PauseSimulation) {
  ASSERT_EQ(state->stepForward(state), Result::OK);

  size_t currentPosition = state->getCurrentInstruction(state);
  ASSERT_EQ(state->pauseSimulation(state), Result::OK);
  ASSERT_EQ(state->runSimulation(state), Result::OK);
  ASSERT_EQ(state->getCurrentInstruction(state), currentPosition)
      << "Simulation with 'run' continued after pause in " << GetParam()
      << "\n";
  ASSERT_EQ(state->stepOverForward(state), Result::OK);
  ASSERT_NE(state->getCurrentInstruction(state), currentPosition)
      << "Simulation still paused after second 'step over' in " << GetParam()
      << "\n";

  currentPosition = state->getCurrentInstruction(state);
  ASSERT_EQ(state->pauseSimulation(state), Result::OK);
  ASSERT_EQ(state->runSimulationBackward(state), Result::OK);
  ASSERT_EQ(state->getCurrentInstruction(state), currentPosition)
      << "Simulation with 'runBackward' continued after pause in " << GetParam()
      << "\n";
  ASSERT_EQ(state->stepOverForward(state), Result::OK);
  ASSERT_NE(state->getCurrentInstruction(state), currentPosition)
      << "Simulation still paused after second 'step over' in " << GetParam()
      << "\n";

  if (GetParam() != "complex-jumps") {
    return;
  }

  // test stepOverForward after pause at CALL instruction (will pause
  // immediately)
  ASSERT_EQ(state->resetSimulation(state), Result::OK);
  ASSERT_EQ(state->stepForward(state), Result::OK);
  ASSERT_EQ(state->stepForward(state), Result::OK);
  ASSERT_EQ(state->stepForward(state), Result::OK);
  ASSERT_EQ(state->stepForward(state), Result::OK);
  ASSERT_EQ(state->pauseSimulation(state), Result::OK);
  ASSERT_EQ(state->stepOverForward(state), Result::OK);
  ASSERT_EQ(state->getCurrentInstruction(state), 12);

  // test stepOutForward after pause inside custom gate (will run one more
  // instruction)
  ASSERT_EQ(state->resetSimulation(state), Result::OK);
  ASSERT_EQ(state->stepForward(state), Result::OK);
  ASSERT_EQ(state->stepForward(state), Result::OK);
  ASSERT_EQ(state->stepForward(state), Result::OK);
  ASSERT_EQ(state->stepForward(state), Result::OK);
  ASSERT_EQ(state->stepForward(state), Result::OK);
  ASSERT_EQ(state->pauseSimulation(state), Result::OK);
  ASSERT_EQ(state->stepOutForward(state), Result::OK);
  ASSERT_EQ(state->getCurrentInstruction(state), 6);

  // test stepOverBackward after pause at RETURN instruction (will pause
  // immediately)
  ASSERT_EQ(state->resetSimulation(state), Result::OK);
  ASSERT_EQ(state->stepForward(state), Result::OK);
  ASSERT_EQ(state->stepForward(state), Result::OK);
  ASSERT_EQ(state->stepForward(state), Result::OK);
  ASSERT_EQ(state->stepForward(state), Result::OK);
  ASSERT_EQ(state->stepForward(state), Result::OK);
  ASSERT_EQ(state->stepForward(state), Result::OK);
  ASSERT_EQ(state->stepOverForward(state), Result::OK);
  ASSERT_EQ(state->pauseSimulation(state), Result::OK);
  ASSERT_EQ(state->stepOverBackward(state), Result::OK);
  ASSERT_EQ(state->getCurrentInstruction(state), 7);

  // test stepOutBackward after pause inside custom gate (will run one more
  // instruction)
  ASSERT_EQ(state->resetSimulation(state), Result::OK);
  ASSERT_EQ(state->stepForward(state), Result::OK);
  ASSERT_EQ(state->stepForward(state), Result::OK);
  ASSERT_EQ(state->stepForward(state), Result::OK);
  ASSERT_EQ(state->stepForward(state), Result::OK);
  ASSERT_EQ(state->stepForward(state), Result::OK);
  ASSERT_EQ(state->stepForward(state), Result::OK);
  ASSERT_EQ(state->pauseSimulation(state), Result::OK);
  ASSERT_EQ(state->stepOutBackward(state), Result::OK);
  ASSERT_EQ(state->getCurrentInstruction(state), 5);
}

/**
 * @test Test the correctness of the `resetSimulation` method.
 *
 * This is done by first running the simulation for a few steps, then resetting
 * it and checking if the simulation starts from the beginning.
 */
TEST_P(SimulationTest, ResetSimulation) {
  for (size_t i = 0; i < 10; i++) {
    ASSERT_EQ(state->stepOverForward(state), Result::OK);
    ASSERT_EQ(state->stepOverForward(state), Result::OK);
    ASSERT_EQ(state->stepOverForward(state), Result::OK);
    ASSERT_EQ(state->resetSimulation(state), Result::OK);
    ASSERT_FALSE(state->canStepBackward(state));
    ASSERT_EQ(state->getCurrentInstruction(state), 0);
    ASSERT_EQ(state->runSimulation(state), Result::OK);
    while (state->didAssertionFail(state)) {
      ASSERT_EQ(state->runSimulation(state), Result::OK);
    }
    ASSERT_FALSE(state->canStepForward(state));
    ASSERT_EQ(state->resetSimulation(state), Result::OK);
    ASSERT_FALSE(state->canStepBackward(state));
    ASSERT_EQ(state->getCurrentInstruction(state), 0);
  }
}

/**
 * @test Test the correctness of the `stepOverForward` and `stepOverBackward`
 * methods.
 *
 * This test steps through the simulation in both directions using different
 * movement types and checks the "step over" methods at different times during
 * execution, such as before a custom gate call, after one, etc.\n\n
 *
 * For `complex-jumps`, this specifically concerns the jumps and calls in the
 * code.\n For `failing-assertions`, this concerns the encountered assertions,
 * making sure that they are identified correctly.
 */
TEST_P(SimulationTest, StepOver) {
  const std::map<const std::string,
                 std::vector<std::pair<const std::string, size_t>>>
      expected = {
          {"complex-jumps",
           {
               {"of", 1},  {"of", 4},  {"sf", 9},  {"sf", 12}, {"of", 13},
               {"ob", 12}, {"sf", 5},  {"of", 6},  {"of", 7},  {"ob", 6},
               {"sf", 10}, {"of", 11}, {"ob", 10}, {"sf", 2},  {"of", 3},
               {"of", 11}, {"of", 7},  {"sb", 11}, {"of", 7},  {"of", 8},
               {"of", 13}, {"sb", 8},  {"of", 13}, {"of", 14},
           }},
          {"failing-assertions",
           {{"of", 1},        {"of", 2},        {"of", 3},
            {"of", 4},        {"of", 5},        {"of", 6},
            {"of", 6},        {"assertion", 6}, {"of", 7},
            {"ob", 6},        {"assertion", 6}, {"of", 7},
            {"ob", 6},        {"assertion", 6}, {"ob", 5},
            {"of", 6},        {"of", 6},        {"assertion", 6},
            {"of", 7},        {"of", 8},        {"sf", 9},
            {"sb", 8},        {"sf", 9},        {"sf", 9},
            {"assertion", 9}, {"sb", 8},        {"sf", 9},
            {"sf", 9},        {"assertion", 9}, {"sf", 10},
            {"sb", 9},        {"assertion", 9}, {"sf", 10}}}};
  moveAndCheck(expected.at(GetParam()));
}

/**
 * @test Test the correctness of the `stepOutForward` and `stepOutBackward`
 * methods.
 *
 * This test steps through the simulation in both directions using different
 * movement types and checks the "step out" methods at different times during
 * execution, such as before a custom gate call, after one, etc.\n\n
 *
 * For `complex-jumps`, this specifically concerns the jumps and calls in the
 * code.\n For `failing-assertions`, this concerns the encountered assertions,
 * making sure that they are identified correctly.
 */
TEST_P(SimulationTest, StepOut) {
  const std::map<const std::string,
                 std::vector<std::pair<const std::string, size_t>>>
      expected = {{"complex-jumps",
                   {{"sf", 1},  {"sf", 4}, {"sf", 9},  {"sf", 12}, {"sf", 5},
                    {"ub", 12}, {"sf", 5}, {"sf", 6},  {"ub", 12}, {"sf", 5},
                    {"of", 6},  {"of", 7}, {"ub", 12}, {"sf", 5},  {"sf", 6},
                    {"sf", 10}, {"ub", 6}, {"sf", 10}, {"sf", 2},  {"sf", 3},
                    {"ub", 10}, {"uf", 7}, {"ob", 6},  {"sf", 10}, {"sf", 2},
                    {"uf", 11}, {"uf", 7}, {"uf", 13}}},
                  {"failing-assertions",
                   {{"sf", 1},
                    {"uf", 6},
                    {"uf", 9},
                    {"uf", 11},
                    {"ub", 0},
                    {"uf", 6},
                    {"ub", 0},
                    {"uf", 6},
                    {"uf", 9}}}};
  moveAndCheck(expected.at(GetParam()));
}

/**
 * @test Test the correctness of the `runSimulation` and `runSimulationBackward`
 * methods.
 *
 * This test steps through the simulation in both directions using different
 * movement types and checks the "run" methods at different times during
 * execution, such as before a custom gate call, after one, etc.\n\n
 *
 * For `complex-jumps`, this specifically concerns the jumps and calls in the
 * code.\n For `failing-assertions`, this concerns the encountered assertions,
 * making sure that they are identified correctly.
 */
TEST_P(SimulationTest, RunSimulation) {
  const std::map<const std::string,
                 std::vector<std::pair<const std::string, size_t>>>
      expected = {{"complex-jumps",
                   {{"sf", 1},
                    {"rf", 15},
                    {"rb", 0},
                    {"sf", 1},
                    {"sf", 4},
                    {"sf", 9},
                    {"sf", 12},
                    {"sf", 5},
                    {"sf", 6},
                    {"rf", 15},
                    {"rb", 0},
                    {"rf", 15}}},
                  {"failing-assertions",
                   {{"sf", 1},
                    {"rf", 6},
                    {"rf", 9},
                    {"rf", 11},
                    {"rb", 0},
                    {"rf", 6},
                    {"rb", 0},
                    {"rf", 6},
                    {"rf", 9},
                    {"rf", 11}}}};
  moveAndCheck(expected.at(GetParam()));
  ASSERT_TRUE(state->isFinished(state));
}

/**
 * @test Test that breakpoints located inside gate definitions are handled
 * correctly.
 *
 * This particularly also checks the interactions of breakpoints with "step out"
 * and "step over" instructions.
 */
TEST_P(SimulationTest, InGateDefinitionBreakpoints) {
  if (GetParam() != "complex-jumps") {
    return;
  }

  const std::map<const std::string, std::vector<size_t>> breakpointPositions = {
      {"complex-jumps", {86, 280, 411}}, {"failing-assertions", {}}};
  const std::map<const std::string, std::vector<size_t>>
      expectedBreakpointPositions = {{"complex-jumps", {2, 7, 11}},
                                     {"failing-assertions", {}}};
  const std::map<const std::string, std::vector<size_t>>
      expectedBreakpointHits = {
          {"complex-jumps", {2, 11, 7, 2, 11, 2, 11, 2, 11}},
          {"failing-assertions", {}}};

  for (size_t index = 0; index < breakpointPositions.at(GetParam()).size();
       index++) {
    const auto breakpoint = breakpointPositions.at(GetParam())[index];
    size_t targetInstruction = 0;
    ASSERT_EQ(state->setBreakpoint(state, breakpoint, &targetInstruction),
              Result::OK)
        << "Failed to set breakpoint at instruction " << breakpoint << " in "
        << GetParam() << "\n";
    ASSERT_EQ(targetInstruction,
              expectedBreakpointPositions.at(GetParam())[index])
        << "Breakpoint set at wrong instruction for breakpoint " << breakpoint
        << " in " << GetParam() << "\n";
  }

  for (const auto instruction : expectedBreakpointHits.at(GetParam())) {
    ASSERT_EQ(state->runSimulation(state), Result::OK)
        << "Failed to run simulation in " << GetParam() << "\n";
    while (state->didAssertionFail(state)) {
      ASSERT_EQ(state->runSimulation(state), Result::OK)
          << "Failed to run simulation in " << GetParam() << "\n";
    }
    ASSERT_EQ(state->getCurrentInstruction(state), instruction)
        << "Breakpoint not hit at expected instruction " << instruction
        << " in " << GetParam() << "\n";
    ASSERT_TRUE(state->wasBreakpointHit(state));
  }

  // Test specific step instructions with breakpoints
  ASSERT_EQ(state->resetSimulation(state), Result::OK);
  ASSERT_EQ(state->runSimulation(state), Result::OK);
  ASSERT_EQ(state->stepOutBackward(state), Result::OK);
  ASSERT_EQ(state->stepOverForward(state), Result::OK);
  ASSERT_EQ(state->getCurrentInstruction(state), 2);
  ASSERT_EQ(state->stepOutForward(state), Result::OK);
  ASSERT_EQ(state->stepOverBackward(state), Result::OK);
  ASSERT_EQ(state->getCurrentInstruction(state), 2);
  ASSERT_EQ(state->runSimulation(state), Result::OK);
  ASSERT_EQ(state->runSimulation(state), Result::OK);
  ASSERT_EQ(state->getCurrentInstruction(state), 7);
  ASSERT_EQ(state->stepBackward(state), Result::OK);
  ASSERT_EQ(state->stepOutForward(state), Result::OK);
  ASSERT_EQ(state->getCurrentInstruction(state), 7);
  ASSERT_EQ(state->stepForward(state), Result::OK);
  ASSERT_EQ(state->stepOutBackward(state), Result::OK);
  ASSERT_EQ(state->getCurrentInstruction(state), 7);

  // Test deleting breakpoints
  ASSERT_EQ(state->clearBreakpoints(state), Result::OK)
      << "Failed to clear breakpoints in " << GetParam() << "\n";
  ASSERT_EQ(state->runSimulationBackward(state), Result::OK);
  ASSERT_FALSE(state->wasBreakpointHit(state))
      << "Breakpoint hit after clearing in " << GetParam() << "\n";
  while (!state->isFinished(state)) {
    ASSERT_EQ(state->runSimulation(state), Result::OK);
    ASSERT_FALSE(state->wasBreakpointHit(state))
        << "Breakpoint hit after clearing in " << GetParam() << "\n";
  }
}

/**
 * @test Test that errors are returned when trying to step forward at the end of
 * the code or backward at the beginning.
 */
TEST_P(SimulationTest, StepAtEnds) {
  size_t errors = 0;
  state->runAll(state, &errors);
  ASSERT_EQ(state->stepOverForward(state), Result::ERROR);
  ASSERT_EQ(state->stepForward(state), Result::ERROR);
  ASSERT_EQ(state->stepOutForward(state), Result::ERROR);
  ASSERT_EQ(state->resetSimulation(state), Result::OK);
  ASSERT_EQ(state->stepOverBackward(state), Result::ERROR);
  ASSERT_EQ(state->stepBackward(state), Result::ERROR);
  ASSERT_EQ(state->stepOutBackward(state), Result::ERROR);
}

/**
 * @test Test that errors are returned when placing breakpoints outside the
 * code.
 */
TEST_P(SimulationTest, BreakpointOutside) {
  size_t location = 0;
  ASSERT_EQ(state->setBreakpoint(state, 9999, &location), ERROR);
}

INSTANTIATE_TEST_SUITE_P(StringParams, SimulationTest,
                         ::testing::Values("complex-jumps",
                                           "failing-assertions"));

} // namespace mqt::debugger::test
