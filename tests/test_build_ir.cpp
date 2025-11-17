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

#include "mock_dag.hpp"

TEST_CASE("build_ir - default policies produce stringified ids", "[build_ir]") {
  // simple chain 0->1->2
  MockDagView g({MockHandle{0}}, {{MockHandle{1}}, {MockHandle{2}}, {}});
  auto ir = dagir::build_ir(g);
  REQUIRE(ir.nodes.size() == 3);
  // labels default to stable_key string
  std::unordered_map<uint64_t, std::string> labels;
  for (auto const& n : ir.nodes) labels.emplace(n.id, n.label);
  REQUIRE(labels[0] == "0");
  REQUIRE(labels[1] == "1");
  REQUIRE(labels[2] == "2");
}

TEST_CASE("build_ir - custom node labeler and edge attributes", "[build_ir]") {
  // graph: 0->1, 0->2
  MockDagView g({MockHandle{0}}, {{MockHandle{1}, MockHandle{2}}, {}, {}});

  auto node_label = [](auto const& h) {
    return std::string("N_") + std::to_string(h.stable_key());
  };
  auto edge_attr = [](auto const& parent, auto const& edge_like) {
    // edge_like is expected to provide target()
    auto child = edge_like.target();
    return std::vector<dagir::IRAttr>{
        {"rel", std::to_string(parent.stable_key()) + "->" + std::to_string(child.stable_key())}};
  };

  auto ir = dagir::build_ir(g, node_label, edge_attr);
  REQUIRE(ir.nodes.size() == 3);
  // node labels applied
  std::unordered_map<uint64_t, std::string> labels;
  for (auto const& n : ir.nodes) labels.emplace(n.id, n.label);
  REQUIRE(labels[0] == "N_0");

  // edges: two outgoing from 0
  std::vector<std::pair<uint64_t, uint64_t>> edges;
  edges.reserve(ir.edges.size());
  std::transform(ir.edges.begin(), ir.edges.end(), std::back_inserter(edges),
                 [](auto const& e) { return std::make_pair(e.source, e.target); });
  REQUIRE(std::find(edges.begin(), edges.end(), std::make_pair(0u, 1u)) != edges.end());
  REQUIRE(std::find(edges.begin(), edges.end(), std::make_pair(0u, 2u)) != edges.end());

  // attributes present for both outgoing edges 0->1 and 0->2
  bool found01 = false;
  bool found02 = false;
  for (auto const& e : ir.edges) {
    if (e.source == 0 && e.target == 1) {
      REQUIRE(!e.attributes.empty());
      REQUIRE(e.attributes[0].key == "rel");
      REQUIRE(e.attributes[0].value == "0->1");
      found01 = true;
    }
    if (e.source == 0 && e.target == 2) {
      REQUIRE(!e.attributes.empty());
      REQUIRE(e.attributes[0].key == "rel");
      REQUIRE(e.attributes[0].value == "0->2");
      found02 = true;
    }
  }
  REQUIRE(found01);
  REQUIRE(found02);
}
