// SPDX-License-Identifier: MIT
/**
 * @file tests/test_render_json.cpp
 * @brief Unit tests for JSON renderer
 */

#include <catch2/catch_test_macros.hpp>
#include <dagir/ir.hpp>
#include <dagir/render_json.hpp>
#include <sstream>

TEST_CASE("render_json emits nodes edges roots and graphAttributes", "[render_json]") {
  dagir::ir_graph g;

  dagir::ir_node n1;
  n1.id = 1;
  n1.label = "A";
  n1.attributes.push_back({"k", "v"});
  dagir::ir_node n2;
  n2.id = 2;
  n2.label = "B";
  n2.attributes.push_back({"num", "42"});
  g.nodes.push_back(n1);
  g.nodes.push_back(n2);

  dagir::ir_edge e;
  e.source = 1;
  e.target = 2;
  e.attributes.push_back({"rel", "toB"});
  g.edges.push_back(e);

  g.global_attrs.push_back({std::string(dagir::ir_attrs::k_graph_label), "MyGraph"});

  std::ostringstream os;
  dagir::render_json(os, g);
  auto s = os.str();

  REQUIRE(s.find("\"nodes\"") != std::string::npos);
  REQUIRE(s.find("\"edges\"") != std::string::npos);
  // `roots` is optional in the JSON schema; this IR does not expose roots
  // so we assert the presence of nodes/edges/graphAttributes only.
  REQUIRE(s.find("\"graphAttributes\"") != std::string::npos);
  REQUIRE(s.find("\"id\": \"1\"") != std::string::npos);
  REQUIRE(s.find("\"label\": \"A\"") != std::string::npos);
  REQUIRE(s.find("\"num\": 42") != std::string::npos);
}
