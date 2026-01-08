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
 * @file Span.hpp
 * @brief Provides a Span class for representing a contiguous range of elements.
 *
 * Accessing pointers from the C-interface like arrays is not compliant with the
 * C++ Core Guidelines. This class provides a safer way to access contiguous
 * ranges of elements.
 */

#pragma once

#include <cstddef>

namespace mqt::debugger {

/**
 * @brief Represents a contiguous range of elements that can be accessed as an
 * array.
 * @tparam T The type of the elements in the span.
 */
template <typename T> class Span {
public:
  /**
   * @brief Constructs a new span from a pointer and a size.
   * @param ptr The pointer to the first element in the span.
   * @param count The number of elements in the span.
   */
  Span(T* ptr, size_t count) : pointer(ptr), spanSize(count) {}

  /**
   * @brief Accesses the element at the given index.
   * @param index The index of the element to access.
   * @return The element at the given index.
   */
  T& operator[](size_t index) const {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return pointer[index];
  }

  /**
   * @brief Returns a pointer to the first element in the span.
   * @return A pointer to the first element in the span.
   */
  T* data() const { return pointer; }

  /**
   * @brief Returns the number of elements in the span.
   * @return The number of elements in the span.
   */
  [[nodiscard]] size_t size() const { return spanSize; }

private:
  /**
   * @brief The pointer on which the span is based.
   */
  T* pointer;

  /**
   * @brief The number of elements in the span, given by the user.
   */
  size_t spanSize;
};

} // namespace mqt::debugger
