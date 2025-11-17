/**
 * @file mock_dag.hpp
 * @brief Utility mock DAG view for unit tests.
 *
 * @details
 * This file defines a MockDagView class that simulates a read-only DAG structure.
 *
 * @copyright
 * © DagIR Contributors. All rights reserved.
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include <algorithm>
#include <cstdint>
#include <dagir/ro_dag_view.hpp>
#include <iterator>
#include <vector>

/**
 * @class MockHandle
 * @brief Minimal handle type for concept testing.
 *
 * @details
 * Implements:
 *  - stable_key() for identity
 *  - debug_address() for diagnostics
 *  - Equality operators
 */
struct MockHandle {
  std::uint64_t id{};
  /// @brief Returns a stable key for memoization.
  constexpr std::uint64_t stable_key() const noexcept { return id; }
  /// @brief Returns a debug address (pointer to self).
  constexpr const void* debug_address() const noexcept { return this; }
  friend constexpr bool operator==(MockHandle a, MockHandle b) noexcept { return a.id == b.id; }
  friend constexpr bool operator!=(MockHandle a, MockHandle b) noexcept { return !(a == b); }
};

/**
 * @class MockEdge
 * @brief Minimal edge wrapper exposing target().
 */
struct MockEdge {
  MockHandle child{};
  /// @brief Returns the child handle.
  constexpr MockHandle target() const noexcept { return child; }
};

/**
 * @class MockDagView
 * @brief Mock adapter modeling ReadOnlyDagView for tests.
 *
 * @details
 * Provides:
 *  - children(handle): range of edges, empty if out-of-bounds.
 *  - roots(): range of handles, empty if roots_ is empty.
 */
class MockDagView {
 public:
  using handle = MockHandle;

  /// @brief Constructs a mock DAG view.
  explicit MockDagView(std::vector<handle> roots, std::vector<std::vector<handle>> adjacency)
      : roots_(std::move(roots)), adj_(std::move(adjacency)) {}

  /**
   * @brief Returns range of children for a given handle.
   * @param h Node handle.
   * @return Range of edges or empty range if index is invalid.
   */
  auto children(handle h) const {
    const size_t idx = static_cast<size_t>(h.id);
    std::vector<MockEdge> out;
    if (idx < adj_.size()) {
      out.reserve(adj_[idx].size());
      std::transform(adj_[idx].begin(), adj_[idx].end(), std::back_inserter(out),
                     [](const handle& hh) { return MockEdge{hh}; });
    }
    return out;
  }

  /**
   * @brief Returns range of roots.
   * @return Range of handles or empty range if roots_ is empty.
   */
  auto roots() const {
    // Return a small vector copy of roots to satisfy range concepts easily.
    std::vector<handle> out = roots_;
    return out;
  }

 private:
  std::vector<handle> roots_;
  std::vector<std::vector<handle>> adj_;
};