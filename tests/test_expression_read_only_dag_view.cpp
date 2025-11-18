/**
 * @file test_expression_read_only_dag_view.cpp
 * @brief Tests for `expression_read_only_dag_view` adapter.
 * © DagIR Contributors. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

#include <catch2/catch_test_macros.hpp>
#include <dagir/utility/expression_parser.hpp>
#include <dagir/utility/expression_read_only_dag_view.hpp>

using namespace dagir::utility;

TEST_CASE("expression_read_only_dag_view models read_only_dag_view and traverses AST",
          "[utility][ro_dag_view]") {
  // Parse a small expression
  auto expr = parse_expression("a AND (NOT b)");
  REQUIRE(expr != nullptr);

  // Adapter should model the concept
  expression_read_only_dag_view view(expr.get());
  STATIC_REQUIRE(dagir::concepts::read_only_dag_view<expression_read_only_dag_view>);

  // Roots should contain the root handle
  auto roots = view.roots();
  REQUIRE(roots.begin() != roots.end());
  auto rootHandle = *roots.begin();

  // Root is an AND node and should have two children
  auto children = view.children(rootHandle);
  std::vector<decltype(children.begin())::value_type> collected;
  for (auto it = children.begin(); it != children.end(); ++it) collected.push_back(*it);
  REQUIRE(collected.size() == 2);

  // One child should be a variable 'a' and the other should be a NOT node
  bool saw_var_a = false;
  bool saw_not = false;
  for (auto& e : collected) {
    auto target = e.target();
    // variable -> no children
    auto ch = view.children(target);
    if (ch.begin() == ch.end()) {
      // variable leaf, try to see if name matches 'a'
      if (auto pv = std::get_if<my_variable>(target.ptr)) {
        if (pv->variable_name == "a") saw_var_a = true;
      }
    } else {
      // should be NOT with one child
      saw_not = true;
      auto seq = view.children(target);
      REQUIRE(std::distance(seq.begin(), seq.end()) == 1);
    }
  }

  REQUIRE(saw_var_a);
  REQUIRE(saw_not);
}
