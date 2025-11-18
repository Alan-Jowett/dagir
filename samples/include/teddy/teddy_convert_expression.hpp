/**
 * @file teddy_convert_expression.hpp
 * @brief Helpers to convert sample expression ASTs to TeDDy BDD diagrams.
 *
 * @details
 * This file provides functions to convert expression ASTs defined in
 * `expression_read_only_dag_view.hpp` into TeDDy BDD diagrams using a
 * `teddy::bdd_manager`.
 *
 * @copyright
 * © DagIR Contributors. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <algorithm>
#include <cctype>
#include <dagir/algorithms.hpp>
#include <functional>
#include <string>
#include <unordered_map>
#include <variant>

#include "../expressions/expression_read_only_dag_view.hpp"
#include "libteddy/core.hpp"

namespace dagir {
namespace utility {

/**
 * @brief Convert an expression AST into a TeDDy BDD diagram.
 *
 * @param mgr Teddy BDD manager used to create diagrams.
 * @param expr Expression AST to convert.
 * @param var_map Mapping from variable names to Teddy variable indices.
 * @return A `teddy::bdd_manager::diagram_t` representing the expression.
 *
 * The resolver treats names like `xN` as index `N`; otherwise names are
 * assigned sequential indices and stored in `var_map`.
 */
inline teddy::bdd_manager::diagram_t convert_expression_to_teddy(
    teddy::bdd_manager& mgr, const dagir::utility::my_expression& expr,
    std::unordered_map<std::string, int>& var_map) {
  using diagram_t = teddy::bdd_manager::diagram_t;

  /**
   * @brief Resolve a variable name to a Teddy variable index.
   */
  auto resolve_var = [&](const std::string& name) -> int {
    if (!name.empty() && name[0] == 'x') {
      const auto begin = name.begin() + 1;
      const auto end = name.end();
      const bool all_digits =
          std::all_of(begin, end, [](unsigned char c) { return std::isdigit(c); });
      if (all_digits && name.size() > 1) {
        try {
          return std::stoi(name.substr(1));
        } catch (...) {
        }
      }
    }

    auto it = var_map.find(name);
    if (it != var_map.end()) return it->second;
    int idx = static_cast<int>(var_map.size());
    var_map.emplace(name, idx);
    return idx;
  };

  /**
   * @brief Visitor used with `std::visit` to convert variant nodes.
   */
  struct visitor {
    teddy::bdd_manager& mgr;
    std::function<int(const std::string&)> resolve_var;

    /** Convert variable node to a Teddy variable diagram. */
    diagram_t operator()(const my_variable& v) {
      int idx = resolve_var(v.variable_name);
      return mgr.variable(idx);
    }
    diagram_t operator()(const my_and& a) {
      auto L = std::visit(*this, *a.left);
      auto R = std::visit(*this, *a.right);
      return mgr.apply<teddy::ops::AND>(L, R);
    }
    diagram_t operator()(const my_or& o) {
      auto L = std::visit(*this, *o.left);
      auto R = std::visit(*this, *o.right);
      return mgr.apply<teddy::ops::OR>(L, R);
    }
    diagram_t operator()(const my_xor& x) {
      auto L = std::visit(*this, *x.left);
      auto R = std::visit(*this, *x.right);
      return mgr.apply<teddy::ops::XOR>(L, R);
    }
    diagram_t operator()(const my_not& n) {
      auto D = std::visit(*this, *n.expr);
      return mgr.apply<teddy::ops::NAND>(D, D);
    }
  } vis{mgr, resolve_var};

  return std::visit(vis, expr);
}

inline teddy::bdd_manager::diagram_t convert_expression_to_teddy(
    teddy::bdd_manager& mgr, const dagir::utility::my_expression_ptr& expr_ptr) {
  std::unordered_map<std::string, int> var_map;
  return convert_expression_to_teddy(mgr, *expr_ptr, var_map);
}

}  // namespace utility
}  // namespace dagir