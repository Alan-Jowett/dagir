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

  const fs::path expr_dir("tests/regression_tests/expressions");
  REQUIRE(fs::exists(expr_dir));

  for (auto const& entry : fs::directory_iterator(expr_dir)) {
    if (!entry.is_regular_file()) continue;
    const auto path = entry.path();
    // Only consider .expr files
    if (path.extension() != ".expr") continue;

    INFO("Expression file: " << path.string());

    // Read expression A from file
    auto exprA = read_expression_from_file(path.string());

    // Build var_map and Teddy manager sized by 32 (growable via resolver in convert)
    std::unordered_map<std::string, int> var_map;
    teddy::bdd_manager mgr(static_cast<int32_t>(32), 1024);
    auto diagA = convert_expression_to_teddy(mgr, *exprA, var_map);

    // Build IR and render expression using render_expr
    std::vector<teddy::bdd_manager::diagram_t::node_t*> rootsA;
    rootsA.push_back(diagA.unsafe_get_root());
    auto var_names = build_var_names(var_map);
    dagir::utility::teddy_read_only_dag_view view(&mgr, &var_names, rootsA);
    dagir::ir_graph ir = dagir::build_ir(view, dagir::utility::teddy_node_attributor{},
                                         dagir::utility::teddy_edge_attributor{});

    std::ostringstream os;
    dagir::render_expr(os, ir);
    std::string rendered = os.str();

    // Parse B
    auto parsedB = parse_expression(rendered);

    // Convert parsed B into diagram using a fresh var_map that maps names to indices from var_names
    std::unordered_map<std::string, int> var_map_b;
    for (size_t i = 0; i < var_names.size(); ++i)
      var_map_b.emplace(var_names[i], static_cast<int>(i));
    auto diagB = convert_expression_to_teddy(mgr, *parsedB, var_map_b);

    // XOR and assert constant false
    auto diagX = mgr.apply<teddy::ops::XOR>(diagA, diagB);
    auto root = diagX.unsafe_get_root();
    REQUIRE(root->is_terminal());
    REQUIRE(root->get_value() == false);
  }
}
