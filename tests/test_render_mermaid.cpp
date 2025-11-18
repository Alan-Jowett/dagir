/**
 * @file tests/test_render_mermaid.cpp
 * @brief Unit test for Mermaid renderer
 *
 * @copyright
 * © DagIR Contributors. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

#include <catch2/catch_test_macros.hpp>
#include <dagir/ir.hpp>
#include <dagir/render_mermaid.hpp>
#include <sstream>

TEST_CASE("render_mermaid outputs nodes, edges, title, and respects rankdir", "[render_mermaid]") {
  dagir::ir_graph g;
  // Add two nodes
  dagir::ir_node n1;
  n1.id = 1;
  n1.label = "Alpha";
  dagir::ir_node n2;
  n2.id = 2;
  n2.label = "Beta";
  g.nodes.push_back(n1);
  g.nodes.push_back(n2);

  // Add an edge 1->2 with label
  dagir::ir_edge e;
  e.source = 1;
  e.target = 2;
  e.attributes.emplace("label", "to B");
  g.edges.push_back(e);

  // Set graph-level attributes: label and rankdir
  g.global_attrs.emplace(std::string(dagir::ir_attrs::k_graph_label), "TestGraph");
  g.global_attrs.emplace(std::string(dagir::ir_attrs::k_rankdir), "LR");

  std::ostringstream os;
  dagir::render_mermaid(os, g, "TestGraph");

  std::string out = os.str();
  REQUIRE(out.find("graph LR") != std::string::npos);
  REQUIRE(out.find("title TestGraph") != std::string::npos);
  REQUIRE((out.find("n1[\"Alpha\"]") != std::string::npos ||
           out.find("n1(\"Alpha\")") != std::string::npos));
  REQUIRE(out.find("n1 -- \"to B\" --> n2") != std::string::npos);
}
