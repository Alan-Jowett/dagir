/**
 * @file test_build_ir.cpp
 * @brief Unit tests for DagIR
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

#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <dagir/algorithms.hpp>
#include <dagir/build_ir.hpp>
#include <sstream>
#include <string>

#include "mock_dag.hpp"

TEST_CASE("build_ir - default policies produce stringified ids", "[build_ir]") {
  // simple chain 0->1->2
  MockDagView g({MockHandle{0}}, {{MockHandle{1}}, {MockHandle{2}}, {}});
  auto ir = dagir::build_ir(g);
  REQUIRE(ir.nodes.size() == 3);
  // labels default to stable_key string
  std::unordered_map<uint64_t, std::string> labels;
  for (auto const& n : ir.nodes) {
    const auto& a = n.attributes;
    labels.emplace(n.id, a.count(dagir::ir_attrs::k_label)
                             ? std::string(a.at(dagir::ir_attrs::k_label))
                             : std::to_string(n.id));
  }
  REQUIRE(labels[0] == "0");
  REQUIRE(labels[1] == "1");
  REQUIRE(labels[2] == "2");
}

TEST_CASE("build_ir - custom node attributor and edge attributes", "[build_ir]") {
  // graph: 0->1, 0->2
  MockDagView g({MockHandle{0}}, {{MockHandle{1}, MockHandle{2}}, {}, {}});

  auto node_attrib = [](auto const& /*view*/, auto const& h) {
    std::unordered_map<std::string, std::string> m;
    m.emplace(std::string(dagir::ir_attrs::k_label), "N_" + std::to_string(h.stable_key()));
    return m;
  };
  auto edge_attr = [](auto const& parent, auto const& edge_like) {
    // edge_like is expected to provide target()
    auto child = edge_like.target();
    std::unordered_map<std::string, std::string> m;
    m.emplace(std::string("rel"), std::to_string(parent.stable_key()) + "->" + std::to_string(child.stable_key()));
    return m;
  };

  auto ir = dagir::build_ir(g, node_attrib, edge_attr);
  REQUIRE(ir.nodes.size() == 3);
  // node labels applied by the attributor
  std::unordered_map<uint64_t, std::string> labels;
  for (auto const& n : ir.nodes) {
    const auto& a = n.attributes;
    labels.emplace(n.id, a.count(dagir::ir_attrs::k_label)
                             ? std::string(a.at(dagir::ir_attrs::k_label))
                             : std::to_string(n.id));
  }
  REQUIRE(labels[0] == "N_0");

  // edges: two outgoing from 0
  std::vector<std::pair<uint64_t, uint64_t>> edges;
  edges.reserve(ir.edges.size());
  std::transform(ir.edges.begin(), ir.edges.end(), std::back_inserter(edges),
                 [](auto const& e) { return std::make_pair(e.source, e.target); });
  using edge_pair_t = decltype(edges)::value_type;
  REQUIRE(std::find(edges.begin(), edges.end(), edge_pair_t{0, 1}) != edges.end());
  REQUIRE(std::find(edges.begin(), edges.end(), edge_pair_t{0, 2}) != edges.end());

  // attributes present for both outgoing edges 0->1 and 0->2
  bool found01 = false;
  bool found02 = false;
  for (auto const& e : ir.edges) {
    if (e.source == 0 && e.target == 1) {
      REQUIRE(!e.attributes.empty());
      REQUIRE(std::string(e.attributes.at("rel")) == "0->1");
      found01 = true;
    }
    if (e.source == 0 && e.target == 2) {
      REQUIRE(!e.attributes.empty());
      REQUIRE(std::string(e.attributes.at("rel")) == "0->2");
      found02 = true;
    }
  }
  REQUIRE(found01);
  REQUIRE(found02);
}

// =============================
// Cycle Detection in build_ir Tests
// =============================

TEST_CASE("build_ir - cycle detection disabled by default (backward compatible)", "[build_ir]") {
  // 0 -> 1 -> 0 (cycle)
  MockDagView g({MockHandle{0}}, {{MockHandle{1}}, {MockHandle{0}}});
  
  // Without cycle detection flag, should throw (backward compatible behavior)
  REQUIRE_THROWS_AS(dagir::build_ir(g), std::runtime_error);
}

TEST_CASE("build_ir - cycle detection enabled handles simple cycle", "[build_ir][cycles]") {
  // 0 -> 1 -> 0 (cycle)
  MockDagView g({MockHandle{0}}, {{MockHandle{1}}, {MockHandle{0}}});
  
  auto node_attr = [](auto const& /*view*/, auto const& h) {
    std::unordered_map<std::string, std::string> m;
    m.emplace(std::string(dagir::ir_attrs::k_label), "Node_" + std::to_string(h.stable_key()));
    return m;
  };
  auto edge_attr = [](auto&&...) { return dagir::ir_attr_map{}; };
  
  // With cycle detection enabled, should succeed
  auto ir = dagir::build_ir(g, node_attr, edge_attr, true);
  
  REQUIRE(ir.nodes.size() == 2);
  REQUIRE(ir.edges.size() == 2);
  
  // Check graph has_cycles flag
  REQUIRE(ir.global_attrs.count(dagir::ir_attrs::k_has_cycles) == 1);
  REQUIRE(std::string(ir.global_attrs.at(dagir::ir_attrs::k_has_cycles)) == "true");
  
  // Check that both nodes have cycle_group attribute
  int nodes_with_cycle_group = 0;
  for (const auto& n : ir.nodes) {
    if (n.attributes.count(dagir::ir_attrs::k_cycle_group)) {
      nodes_with_cycle_group++;
    }
  }
  REQUIRE(nodes_with_cycle_group == 2);
  
  // Check that at least one edge is marked as back-edge
  bool found_back_edge = false;
  for (const auto& e : ir.edges) {
    if (e.attributes.count(dagir::ir_attrs::k_is_cycle_back_edge)) {
      found_back_edge = true;
      REQUIRE(std::string(e.attributes.at(dagir::ir_attrs::k_is_cycle_back_edge)) == "true");
    }
  }
  REQUIRE(found_back_edge);
}

TEST_CASE("build_ir - cycle detection enabled with DAG shows no cycles", "[build_ir][cycles]") {
  // 0 -> 1 -> 2 (simple chain, no cycles)
  MockDagView g({MockHandle{0}}, {{MockHandle{1}}, {MockHandle{2}}, {}});
  
  auto node_attr = [](auto const& /*view*/, auto const& h) {
    std::unordered_map<std::string, std::string> m;
    m.emplace(std::string(dagir::ir_attrs::k_label), "Node_" + std::to_string(h.stable_key()));
    return m;
  };
  auto edge_attr = [](auto&&...) { return dagir::ir_attr_map{}; };
  
  // With cycle detection enabled on a DAG
  auto ir = dagir::build_ir(g, node_attr, edge_attr, true);
  
  REQUIRE(ir.nodes.size() == 3);
  REQUIRE(ir.edges.size() == 2);
  
  // Check graph has_cycles flag is not present (no cycles)
  REQUIRE(ir.global_attrs.count(dagir::ir_attrs::k_has_cycles) == 0);
  
  // Check that no edges are marked as back-edges
  for (const auto& e : ir.edges) {
    REQUIRE(e.attributes.count(dagir::ir_attrs::k_is_cycle_back_edge) == 0);
  }
}

TEST_CASE("build_ir - self-loop detected and marked", "[build_ir][cycles]") {
  // 0 -> 0 (self-loop)
  MockDagView g({MockHandle{0}}, {{MockHandle{0}}});
  
  auto node_attr = [](auto const& /*view*/, auto const& h) {
    std::unordered_map<std::string, std::string> m;
    m.emplace(std::string(dagir::ir_attrs::k_label), "Node_" + std::to_string(h.stable_key()));
    return m;
  };
  auto edge_attr = [](auto&&...) { return dagir::ir_attr_map{}; };
  
  // With cycle detection enabled
  auto ir = dagir::build_ir(g, node_attr, edge_attr, true);
  
  REQUIRE(ir.nodes.size() == 1);
  REQUIRE(ir.edges.size() == 1);
  
  // Check graph has_cycles flag
  REQUIRE(ir.global_attrs.count(dagir::ir_attrs::k_has_cycles) == 1);
  
  // Check that the edge is marked as back-edge
  REQUIRE(ir.edges[0].attributes.count(dagir::ir_attrs::k_is_cycle_back_edge) == 1);
}

TEST_CASE("build_ir - complex cycle with partial DAG", "[build_ir][cycles]") {
  // 0 -> 1 -> 2 -> 1 (cycle between 1 and 2), 0 not in cycle
  MockDagView g({MockHandle{0}}, {{MockHandle{1}}, {MockHandle{2}}, {MockHandle{1}}});
  
  auto node_attr = [](auto const& /*view*/, auto const& h) {
    std::unordered_map<std::string, std::string> m;
    m.emplace(std::string(dagir::ir_attrs::k_label), "Node_" + std::to_string(h.stable_key()));
    return m;
  };
  auto edge_attr = [](auto&&...) { return dagir::ir_attr_map{}; };
  
  auto ir = dagir::build_ir(g, node_attr, edge_attr, true);
  
  REQUIRE(ir.nodes.size() == 3);
  REQUIRE(ir.edges.size() == 3);
  
  // Check graph has_cycles flag
  REQUIRE(ir.global_attrs.count(dagir::ir_attrs::k_has_cycles) == 1);
  
  // Check that nodes 1 and 2 have the same cycle_group
  std::string_view group1, group2;
  for (const auto& n : ir.nodes) {
    if (n.id == 1 && n.attributes.count(dagir::ir_attrs::k_cycle_group)) {
      group1 = n.attributes.at(dagir::ir_attrs::k_cycle_group);
    }
    if (n.id == 2 && n.attributes.count(dagir::ir_attrs::k_cycle_group)) {
      group2 = n.attributes.at(dagir::ir_attrs::k_cycle_group);
    }
  }
  REQUIRE(!group1.empty());
  REQUIRE(!group2.empty());
  REQUIRE(group1 == group2);
}

