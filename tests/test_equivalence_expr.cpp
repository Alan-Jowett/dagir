/* SPDX-License-Identifier: MIT */
/**
 * @file tests/test_equivalence_expr.cpp
 * @brief Equivalence test: A XOR render_expr(A) should be constant false
 */

#include <catch2/catch_test_macros.hpp>
#include <dagir/build_ir.hpp>
#include <dagir/render_expr.hpp>
#include <dagir/utility/expressions/expression_parser.hpp>
#include <dagir/utility/teddy/teddy_convert_expression.hpp>
#include <dagir/utility/teddy/teddy_policy.hpp>
#include <dagir/utility/teddy/teddy_read_only_dag_view.hpp>
#include <sstream>

// This test constructs a simple expression A (e.g. "p AND q"), builds a
// Teddy BDD for A, emits the expression text using `render_expr` (B), parses
// B back into an AST, converts that AST into a second Teddy BDD, then builds
// an XOR of the two diagrams and checks the resulting diagram is the
// constant-false terminal.

TEST_CASE("render_expr equivalence produces identical BDD", "[equivalence]") {
  using namespace dagir::utility;

  // Build AST A programmatically: (p AND q)
  auto p = std::make_unique<my_expression>(my_variable{std::string("p")});
  auto q = std::make_unique<my_expression>(my_variable{std::string("q")});
  auto and_expr = std::make_unique<my_expression>(my_and{std::move(p), std::move(q)});

  // Convert A to a Teddy diagram
  std::unordered_map<std::string, int> var_map;
  teddy::bdd_manager mgr(static_cast<int32_t>(2), 1024);
  auto diagA = convert_expression_to_teddy(mgr, *and_expr, var_map);

  // Build a read-only view and IR for A, then render expression B using render_expr
  std::vector<teddy::bdd_manager::diagram_t::node_t*> rootsA;
  rootsA.push_back(diagA.unsafe_get_root());
  auto var_names = std::vector<std::string>{"p", "q"};
  dagir::utility::teddy_read_only_dag_view view(&mgr, &var_names, rootsA);
  dagir::ir_graph ir = dagir::build_ir(view, dagir::utility::teddy_node_attributor{},
                                       dagir::utility::teddy_edge_attributor{});

  std::ostringstream os;
  dagir::render_expr(os, ir);
  std::string rendered = os.str();

  // Parse rendered expression back into AST B
  auto parsedB = parse_expression(rendered);

  // Convert parsed B into a second Teddy diagram (fresh var_map to ensure indices match names)
  std::unordered_map<std::string, int> var_map_b;
  var_map_b.emplace("p", 0);
  var_map_b.emplace("q", 1);
  auto diagB = convert_expression_to_teddy(mgr, *parsedB, var_map_b);

  // XOR the two diagrams: diagA XOR diagB
  auto diagX = mgr.apply<teddy::ops::XOR>(diagA, diagB);

  // The resulting diagram should be the constant false terminal: check root is terminal and
  // value==0
  auto root = diagX.unsafe_get_root();
  REQUIRE(root->is_terminal());
  REQUIRE(root->get_value() == false);
}
