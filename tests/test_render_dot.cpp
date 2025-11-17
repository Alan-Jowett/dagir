/**
 * @file test_render_dot.cpp
 * @brief Unit tests for the GraphViz DOT renderer (`dagir::render_dot`).
 *
 * @details
 * This test verifies that a small `dagir::ir_graph` is rendered to a
 * valid GraphViz DOT snippet and that node/edge attributes appear as expected.
 *
 * SPDX-License-Identifier: MIT
 */

#include <catch2/catch_test_macros.hpp>
#include <dagir/ir.hpp>
#include <dagir/render_dot.hpp>
#include <sstream>

TEST_CASE("render_dot outputs nodes and edges with attributes", "[render_dot]") {
  dagir::ir_graph g;

  // Create two nodes (avoid designated initializers for MSVC compatibility)
  dagir::ir_node a;
  a.id = 1;
  a.label = "Alpha";
  a.attributes.push_back({std::string(dagir::ir_attrs::k_fill_color), "#ff0000"});

  dagir::ir_node b;
  b.id = 2;
  b.label = "Beta";
  b.attributes.push_back({std::string(dagir::ir_attrs::k_shape), "box"});

  g.nodes.push_back(a);
  g.nodes.push_back(b);

  // Edge with label
  dagir::ir_edge e;
  e.source = 1;
  e.target = 2;
  e.attributes.push_back({std::string(dagir::ir_attrs::k_label), "to B"});
  g.edges.push_back(e);

  std::ostringstream oss;
  dagir::render_dot(oss, g, "TestGraph");

  const std::string out = oss.str();

  REQUIRE(out.find("digraph TestGraph") != std::string::npos);
  REQUIRE(out.find("n1 [label=\"Alpha\"") != std::string::npos);
  REQUIRE(out.find("n2 [label=\"Beta\"") != std::string::npos);
  REQUIRE(out.find("n1 -> n2") != std::string::npos);
  REQUIRE(out.find("label=\"to B\"") != std::string::npos);
  // fillcolor should have produced style=filled implicitly
  REQUIRE(out.find("style=\"filled\"") != std::string::npos);
}
