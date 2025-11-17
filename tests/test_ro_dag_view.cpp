/**
 * @file test_ro_dag_view.cpp
 * @brief Unit tests for DagIR read_only_dag_view concept and related utilities.
 *
 * @details
 * This test suite validates:
 *  - Compliance of mock types with DagIR concepts (`node_handle`, `edge_ref`,
 * `read_only_dag_view`).
 *  - Safe bounds checking for children() and roots() to prevent undefined behavior.
 *  - Utility helpers like `basic_edge` and `models_read_only_view()`.
 *
 * @copyright
 * © DagIR Contributors. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

#include <catch2/catch_test_macros.hpp>

#include "mock_dag.hpp"

// -----------------------------
// Unit Tests
// -----------------------------

/**
 * @test Verify `node_handle` concept compliance.
 */
TEST_CASE("MockHandle satisfies node_handle concept", "[concepts]") {
  STATIC_REQUIRE(dagir::concepts::node_handle<MockHandle>);
  MockHandle h{42};
  REQUIRE(h.stable_key() == 42);
  REQUIRE(h.debug_address() == &h);
}

/**
 * @test Verify `edge_ref` concept compliance.
 */
TEST_CASE("MockEdge satisfies edge_ref concept", "[concepts]") {
  STATIC_REQUIRE(dagir::concepts::edge_ref<MockEdge, MockHandle>);
  MockHandle h{7};
  MockEdge e{h};
  REQUIRE(e.target().stable_key() == 7);
}

/**
 * @test Verify `read_only_dag_view` concept compliance and helper.
 */
TEST_CASE("MockDagView satisfies read_only_dag_view concept", "[concepts]") {
  STATIC_REQUIRE(dagir::concepts::read_only_dag_view<MockDagView>);
  STATIC_REQUIRE(dagir::models_read_only_view<MockDagView>());

  MockHandle root{0}, child{1};
  MockDagView view({root}, {{child}, {}});

  auto roots = view.roots();
  REQUIRE((*roots.begin()).stable_key() == 0);

  auto children = view.children(root);
  REQUIRE((*children.begin()).target().stable_key() == 1);

  // Bounds check: invalid handle should yield empty range
  MockHandle invalid{99};
  auto emptyChildren = view.children(invalid);
  REQUIRE(emptyChildren.begin() == emptyChildren.end());
}

/**
 * @test Verify empty roots returns empty range.
 */
TEST_CASE("Empty roots returns empty range", "[bounds]") {
  MockDagView emptyView({}, {});
  auto emptyRoots = emptyView.roots();
  REQUIRE(emptyRoots.begin() == emptyRoots.end());
}

/**
 * @test Verify `basic_edge` utility works with `node_handle`.
 */
TEST_CASE("basic_edge wrapper returns correct target", "[utility]") {
  dagir::basic_edge<MockHandle> edge{MockHandle{99}};
  REQUIRE(edge.target().stable_key() == 99);
}
