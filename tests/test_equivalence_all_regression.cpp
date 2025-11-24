/* SPDX-License-Identifier: MIT */
/**
 * @file tests/test_equivalence_all_regression.cpp
 * @brief Run equivalence check for all regression expressions in tests/regression_tests/expressions
 */

#include <catch2/catch_test_macros.hpp>
#include <dagir/build_ir.hpp>
#include <dagir/render_expr.hpp>
#include <dagir/utility/expressions/expression_parser.hpp>
#include <dagir/utility/teddy/teddy_convert_expression.hpp>
#include <dagir/utility/teddy/teddy_policy.hpp>
#include <dagir/utility/teddy/teddy_read_only_dag_view.hpp>
#include <filesystem>
#include <sstream>

TEST_CASE("render_expr equivalence over regression directory", "[equivalence][regression]") {
  using namespace dagir::utility;
  namespace fs = std::filesystem;

  // Determine regression expressions directory relative to this source file
  const fs::path source_dir = fs::path(__FILE__).parent_path();
  const fs::path expr_dir = source_dir / "regression_tests" / "expressions";
  (void)source_dir;
  (void)expr_dir;
  REQUIRE(fs::exists(expr_dir));

  for (auto const& entry : fs::directory_iterator(expr_dir)) {
    if (!entry.is_regular_file()) continue;
    const auto path = entry.path();
    // Only consider .expr files
    if (path.extension() != ".expr") continue;

    // Process all regression files

    INFO("Expression file: " << path.string());

    // Read expression A from file
    auto exprA = read_expression_from_file(path.string());

    // Build var_map and Teddy manager with generous initial variable capacity
    std::unordered_map<std::string, int> var_map;
    // Use larger initial variable count (128) to avoid manager growth bugs
    // on large regression expressions (e.g., 6-Queens uses >32 variables).
    teddy::bdd_manager mgr(static_cast<int32_t>(128), 1024);
    auto diagA = convert_expression_to_teddy(mgr, *exprA, var_map);
    auto rootA = diagA.unsafe_get_root();
    // Build index->name vector from var_map (same logic as examples/ expression2bdd)
    std::vector<std::string> var_names;
    var_names.resize(var_map.size());
    for (auto const& kv : var_map) {
      const auto& name = kv.first;
      const auto idx = static_cast<size_t>(kv.second);
      if (idx < var_names.size()) var_names[idx] = name;
    }
    REQUIRE(rootA != nullptr);

    // Build IR and render expression using render_expr
    std::vector<teddy::bdd_manager::diagram_t::node_t*> rootsA;
    rootsA.push_back(diagA.unsafe_get_root());
    dagir::utility::teddy_read_only_dag_view view(&mgr, &var_names, rootsA);
    (void)rootsA;
    dagir::ir_graph ir = dagir::build_ir(view, dagir::utility::teddy_node_attributor{},
                                         dagir::utility::teddy_edge_attributor{});
    std::ostringstream os;
    dagir::render_expr(os, ir);
    std::string rendered = os.str();

    // Parse B
    auto parsedB = parse_expression(rendered);
    (void)parsedB;

    // Convert parsed B into diagram using a fresh var_map that maps names to indices from var_names
    std::unordered_map<std::string, int> var_map_b;
    for (size_t i = 0; i < var_names.size(); ++i)
      var_map_b.emplace(var_names[i], static_cast<int>(i));
    auto diagB = convert_expression_to_teddy(mgr, *parsedB, var_map_b);
    auto rootB = diagB.unsafe_get_root();
    REQUIRE(rootB != nullptr);

    // XOR and assert constant false
    auto diagX = mgr.apply<teddy::ops::XOR>(diagA, diagB);
    auto root = diagX.unsafe_get_root();
    REQUIRE(root != nullptr);
    REQUIRE(root->is_terminal());
    REQUIRE(root->get_value() == false);
  }
}
