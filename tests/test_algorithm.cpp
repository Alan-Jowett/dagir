/**
 * @file test_algorithm.cpp
 * @brief Unit tests for DagIR algorithm utilities.
 *
 * @details
 * This test suite validates:
 * - Correctness of Kahn's topological sort implementation.
 * - Proper handling of cycles in the DAG.
 * - Correctness of postorder folding over the DAG.
 * - Edge cases and error handling.
 *
 * @copyright
 * © DagIR Contributors. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

#include <catch2/catch_test_macros.hpp>
#include <dagir/algorithms.hpp>

#include "mock_dag.hpp"
#include <numeric>

// -----------------------------
using dagir::kahn_topological_order;
using dagir::postorder_fold;

TEST_CASE("kahn_topological_order - simple chain", "[algorithms]") {
  // 0 -> 1 -> 2
  MockDagView g({MockHandle{0}}, {{MockHandle{1}}, {MockHandle{2}}, {}});
  auto order = kahn_topological_order(g);
  REQUIRE(order.size() == 3);
  // stable_key() returns id
  REQUIRE(order[0].stable_key() == 0);
  REQUIRE(order[1].stable_key() == 1);
  REQUIRE(order[2].stable_key() == 2);
}

TEST_CASE("kahn_topological_order - multiple roots and branching", "[algorithms]") {
  // Roots: 0, 1 ; edges: 0 -> 2, 1 -> 2, 2 -> 3
  MockDagView g({MockHandle{0}, MockHandle{1}},
                {{MockHandle{2}}, {MockHandle{2}}, {MockHandle{3}}, {}});
  auto order = kahn_topological_order(g);
  REQUIRE(order.size() == 4);
  // first two elements should be 0 and 1 (order between them unspecified)
  std::unordered_set<std::uint64_t> first_two = {order[0].stable_key(), order[1].stable_key()};
  REQUIRE(first_two.count(0));
  REQUIRE(first_two.count(1));
  REQUIRE(order[2].stable_key() == 2);
  REQUIRE(order[3].stable_key() == 3);
}

TEST_CASE("kahn_topological_order - cycle detection", "[algorithms]") {
  // 0 -> 1 -> 0 (cycle)
  MockDagView g({MockHandle{0}}, {{MockHandle{1}}, {MockHandle{0}}});
  REQUIRE_THROWS_AS(kahn_topological_order(g), std::runtime_error);
}

TEST_CASE("postorder_fold - sum of child results + node id", "[algorithms]") {
  // 0 -> 1 -> 2 ; values: use id as base value
  MockDagView g({MockHandle{0}}, {{MockHandle{1}}, {MockHandle{2}}, {}});

  auto combiner = [](auto const& /*view*/, MockHandle node, std::span<const int> children) -> int {
    int sum = static_cast<int>(node.stable_key());
    sum += std::accumulate(children.begin(), children.end(), 0);
    return sum;
  };

  auto results = postorder_fold<MockDagView, int>(g, combiner);
  // node 2: 2
  REQUIRE(results.at(2) == 2);
  // node 1: 1 + child(2) = 3
  REQUIRE(results.at(1) == 3);
  // node 0: 0 + child(1) = 3
  REQUIRE(results.at(0) == 3);
}
