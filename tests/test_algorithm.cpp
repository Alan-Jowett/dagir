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
#include <numeric>

#include "mock_dag.hpp"

// -----------------------------
using dagir::kahn_topological_order;
using dagir::postorder_fold;
using dagir::detect_cycles;
using dagir::dfs_traversal_order;

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

// =============================
// Cycle Detection Tests
// =============================

TEST_CASE("detect_cycles - simple DAG has no cycles", "[algorithms][cycles]") {
  // 0 -> 1 -> 2
  MockDagView g({MockHandle{0}}, {{MockHandle{1}}, {MockHandle{2}}, {}});
  auto info = detect_cycles(g);
  
  REQUIRE(info.has_cycles == false);
  REQUIRE(info.back_edges.empty());
  REQUIRE(info.num_sccs == 3);  // Each node is its own SCC in a DAG
}

TEST_CASE("detect_cycles - simple cycle detected", "[algorithms][cycles]") {
  // 0 -> 1 -> 0 (cycle)
  MockDagView g({MockHandle{0}}, {{MockHandle{1}}, {MockHandle{0}}});
  auto info = detect_cycles(g);
  
  REQUIRE(info.has_cycles == true);
  REQUIRE(info.back_edges.size() >= 1);
  REQUIRE(info.num_sccs >= 1);
  
  // Both nodes should be in the same SCC
  REQUIRE(info.cycle_groups.at(0) == info.cycle_groups.at(1));
}

TEST_CASE("detect_cycles - self-loop", "[algorithms][cycles]") {
  // 0 -> 0 (self-loop)
  MockDagView g({MockHandle{0}}, {{MockHandle{0}}});
  auto info = detect_cycles(g);
  
  REQUIRE(info.has_cycles == true);
  REQUIRE(info.back_edges.size() >= 1);
  
  // Check that the back-edge is detected
  bool found_back_edge = false;
  for (const auto& [edge, is_back] : info.back_edges) {
    if (edge.first == 0 && edge.second == 0 && is_back) {
      found_back_edge = true;
      break;
    }
  }
  REQUIRE(found_back_edge);
}

TEST_CASE("detect_cycles - complex cycle with multiple SCCs", "[algorithms][cycles]") {
  // 0 -> 1 -> 2 -> 1 (cycle between 1 and 2)
  // 0 is not in the cycle
  MockDagView g({MockHandle{0}}, {{MockHandle{1}}, {MockHandle{2}}, {MockHandle{1}}});
  auto info = detect_cycles(g);
  
  REQUIRE(info.has_cycles == true);
  REQUIRE(info.back_edges.size() >= 1);
  
  // Nodes 1 and 2 should be in the same SCC
  REQUIRE(info.cycle_groups.at(1) == info.cycle_groups.at(2));
  // Node 0 should be in a different SCC
  REQUIRE(info.cycle_groups.at(0) != info.cycle_groups.at(1));
}

TEST_CASE("detect_cycles - diamond DAG has no cycles", "[algorithms][cycles]") {
  // 0 -> 1, 0 -> 2, 1 -> 3, 2 -> 3 (diamond shape, no cycles)
  MockDagView g({MockHandle{0}}, 
                {{MockHandle{1}, MockHandle{2}}, 
                 {MockHandle{3}}, 
                 {MockHandle{3}}, 
                 {}});
  auto info = detect_cycles(g);
  
  REQUIRE(info.has_cycles == false);
  REQUIRE(info.back_edges.empty());
}

// =============================
// DFS Traversal Tests
// =============================

TEST_CASE("dfs_traversal_order - simple chain DAG", "[algorithms][dfs]") {
  // 0 -> 1 -> 2
  MockDagView g({MockHandle{0}}, {{MockHandle{1}}, {MockHandle{2}}, {}});
  auto order = dfs_traversal_order(g);
  
  REQUIRE(order.size() == 3);
  // In DFS postorder (reversed), we should get 0, 1, 2
  REQUIRE(order[0].stable_key() == 0);
  REQUIRE(order[1].stable_key() == 1);
  REQUIRE(order[2].stable_key() == 2);
}

TEST_CASE("dfs_traversal_order - handles simple cycle", "[algorithms][dfs][cycles]") {
  // 0 -> 1 -> 0 (cycle)
  MockDagView g({MockHandle{0}}, {{MockHandle{1}}, {MockHandle{0}}});
  auto order = dfs_traversal_order(g);
  
  // Should visit both nodes without throwing
  REQUIRE(order.size() == 2);
  
  // Collect the keys
  std::unordered_set<std::uint64_t> visited;
  for (const auto& h : order) {
    visited.insert(h.stable_key());
  }
  
  REQUIRE(visited.count(0) == 1);
  REQUIRE(visited.count(1) == 1);
}

TEST_CASE("dfs_traversal_order - handles self-loop", "[algorithms][dfs][cycles]") {
  // 0 -> 0 (self-loop)
  MockDagView g({MockHandle{0}}, {{MockHandle{0}}});
  auto order = dfs_traversal_order(g);
  
  // Should visit the node once
  REQUIRE(order.size() == 1);
  REQUIRE(order[0].stable_key() == 0);
}

TEST_CASE("dfs_traversal_order - complex DCG", "[algorithms][dfs][cycles]") {
  // 0 -> 1 -> 2 -> 1 (cycle between 1 and 2), 0 not in cycle
  MockDagView g({MockHandle{0}}, {{MockHandle{1}}, {MockHandle{2}}, {MockHandle{1}}});
  auto order = dfs_traversal_order(g);
  
  // Should visit all nodes
  REQUIRE(order.size() == 3);
  
  // Collect the keys
  std::unordered_set<std::uint64_t> visited;
  for (const auto& h : order) {
    visited.insert(h.stable_key());
  }
  
  REQUIRE(visited.count(0) == 1);
  REQUIRE(visited.count(1) == 1);
  REQUIRE(visited.count(2) == 1);
}

TEST_CASE("dfs_traversal_order - multiple roots", "[algorithms][dfs]") {
  // Roots: 0, 1; 0 -> 2, 1 -> 2
  MockDagView g({MockHandle{0}, MockHandle{1}},
                {{MockHandle{2}}, {MockHandle{2}}, {}});
  auto order = dfs_traversal_order(g);
  
  REQUIRE(order.size() == 3);
  
  // All nodes should be visited
  std::unordered_set<std::uint64_t> visited;
  for (const auto& h : order) {
    visited.insert(h.stable_key());
  }
  
  REQUIRE(visited.count(0) == 1);
  REQUIRE(visited.count(1) == 1);
  REQUIRE(visited.count(2) == 1);
}

