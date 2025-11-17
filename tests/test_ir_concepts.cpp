/**
 * @file test_ir_concepts.cpp
 * @brief Unit tests for DagIR concepts related to IR building.
 *
 * @details
 * This test suite validates:
 *  - That the Mock types satisfy basic concepts used by the IR builder.
 *  - That the convenience `build_ir(view)` overload compiles and returns
 *    a sensible `ir_graph` for a trivial DAG.
 *
 * SPDX-License-Identifier: MIT
 */

#include <catch2/catch_test_macros.hpp>
#include <dagir/build_ir.hpp>
#include <dagir/concepts/edge_ref.hpp>
#include <dagir/concepts/node_handle.hpp>

#include "mock_dag.hpp"

// -----------------------------

TEST_CASE("Mock types satisfy basic concepts", "[concepts]") {
  STATIC_REQUIRE(dagir::concepts::node_handle<MockHandle>);
  STATIC_REQUIRE(dagir::concepts::edge_ref<MockEdge, MockHandle>);
  STATIC_REQUIRE(dagir::concepts::read_only_dag_view<MockDagView>);
}

TEST_CASE("Default build_ir overload constructs graph", "[build_ir]") {
  MockHandle root{0}, child{1};
  MockDagView view({root}, {{child}, {}});

  // Should compile and produce a graph with a single node and one edge
  auto g = dagir::build_ir(view);
  REQUIRE(!g.nodes.empty());
  REQUIRE(!g.edges.empty());
  REQUIRE(g.nodes.front().id == root.stable_key());
  REQUIRE(g.edges.front().source == root.stable_key());
  REQUIRE(g.edges.front().target == child.stable_key());
}
