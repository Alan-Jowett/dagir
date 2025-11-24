// SPDX-License-Identifier: MIT
// Copyright (c) 2025 DagIR contributors

#include <catch2/catch_test_macros.hpp>
#include <dagir/ir.hpp>
#include <dagir/ir_attrs.hpp>
#include <dagir/ir_serialization.hpp>

using namespace dagir;

TEST_CASE("ir_graph round-trip serialization") {
  ir_graph g;
  // Create two nodes
  ir_node a;
  a.id = 1;
  a.attributes[g.attr_cache.cache_view(ir_attrs::k_name)] = g.attr_cache.cache_view("A");
  a.attributes[g.attr_cache.cache_view(ir_attrs::k_label)] = g.attr_cache.cache_view("node A");

  ir_node b;
  b.id = 2;
  b.attributes[g.attr_cache.cache_view(ir_attrs::k_name)] = g.attr_cache.cache_view("B");
  b.attributes[g.attr_cache.cache_view(ir_attrs::k_label)] = g.attr_cache.cache_view("node B");

  g.nodes.push_back(std::move(a));
  g.nodes.push_back(std::move(b));

  ir_edge e;
  e.source = 1;
  e.target = 2;
  e.attributes[g.attr_cache.cache_view("weight")] = g.attr_cache.cache_view("42");
  g.edges.push_back(std::move(e));

  // roots
  g.roots.push_back(g.attr_cache.cache_view("A"));

  // Serialize and deserialize
  std::string json = dagir::serialize::to_json(g);
  dagir::ir_graph g2 = dagir::serialize::from_json(json);

  REQUIRE(g2.nodes.size() == 2);
  REQUIRE(g2.edges.size() == 1);
}

TEST_CASE("ir_serialization error cases") {
  // Missing keys
  std::string bad = "{\"schema_version\":1}";
  REQUIRE_THROWS_AS(dagir::serialize::from_json(bad), std::invalid_argument);
}
