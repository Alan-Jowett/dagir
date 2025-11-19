/**
 * @file cudd_convert_expression.hpp
 * @brief Helpers to convert sample expression ASTs to CUDD BDD diagrams.
 *
 * @details
 * This file provides functions to convert expression ASTs defined in
 * `expression_read_only_dag_view.hpp` into CUDD BDD diagrams using a
 * `DdManager`.
 *
 * @copyright
 * © DagIR Contributors. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cudd/cudd.h>

#include <algorithm>
#include <cctype>
#include <dagir/algorithms.hpp>
#include <dagir/utility/expressions/expression_read_only_dag_view.hpp>
#include <functional>
#include <string>
#include <unordered_map>
#include <variant>

namespace dagir {
namespace utility {

inline DdNode* convert_expression_to_cudd(DdManager& mgr, const dagir::utility::my_expression& expr,
                                          std::unordered_map<std::string, int>& var_map) {
  auto resolve_var = [&](const std::string& name) -> int {
    auto it = var_map.find(name);
    if (it != var_map.end()) return it->second;
    int idx = static_cast<int>(var_map.size());
    var_map.emplace(name, idx);
    return idx;
  };

  struct visitor {
    DdManager& mgr;
    std::function<int(const std::string&)> resolve_var;

    DdNode* operator()(const my_variable& v) {
      int idx = resolve_var(v.variable_name);
      DdNode* var = Cudd_bddIthVar(&mgr, idx);
      Cudd_Ref(var);
      return var;
    }
    DdNode* operator()(const my_and& a) {
      DdNode* L = std::visit(*this, *a.left);
      DdNode* R = std::visit(*this, *a.right);
      DdNode* out = Cudd_bddAnd(&mgr, L, R);
      Cudd_Ref(out);
      Cudd_RecursiveDeref(&mgr, L);
      Cudd_RecursiveDeref(&mgr, R);
      return out;
    }
    DdNode* operator()(const my_or& o) {
      DdNode* L = std::visit(*this, *o.left);
      DdNode* R = std::visit(*this, *o.right);
      DdNode* out = Cudd_bddOr(&mgr, L, R);
      Cudd_Ref(out);
      Cudd_RecursiveDeref(&mgr, L);
      Cudd_RecursiveDeref(&mgr, R);
      return out;
    }
    DdNode* operator()(const my_xor& x) {
      DdNode* L = std::visit(*this, *x.left);
      DdNode* R = std::visit(*this, *x.right);
      DdNode* out = Cudd_bddXor(&mgr, L, R);
      Cudd_Ref(out);
      Cudd_RecursiveDeref(&mgr, L);
      Cudd_RecursiveDeref(&mgr, R);
      return out;
    }
    DdNode* operator()(const my_not& n) {
      DdNode* D = std::visit(*this, *n.expr);
      DdNode* out = Cudd_Not(D);
      // Cudd_Not does not require ref/ deref when applied to referenced nodes,
      // ensure the returned node is referenced by the caller instead.
      Cudd_Ref(out);
      Cudd_RecursiveDeref(&mgr, D);
      return out;
    }
  } vis{mgr, resolve_var};

  DdNode* res = std::visit(vis, expr);
  return res;
}

inline DdNode* convert_expression_to_cudd(DdManager& mgr,
                                          const dagir::utility::my_expression_ptr& expr_ptr) {
  std::unordered_map<std::string, int> var_map;
  return convert_expression_to_cudd(mgr, *expr_ptr, var_map);
}

}  // namespace utility
}  // namespace dagir
