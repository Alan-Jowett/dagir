/* SPDX-License-Identifier: MIT */
/**
 * @file render_expr.hpp
 * @brief Render an IR graph (BDD) as an equivalent boolean expression.
 *
 * Produces a textual boolean expression using the keywords `NOT`, `AND`, and
 * `OR`. Parentheses are added where necessary to preserve operator
 * precedence (NOT > AND > OR). The implementation expects the input
 * `ir_graph` to represent a BDD produced by the provided TeDDy/CUDD
 * policies: non-terminal nodes carry a `label` attribute with the
 * variable name and outgoing edges are labelled with `style` ==
 * "solid" for the true branch and "dashed" for the false branch.
 */

#pragma once

#include <algorithm>
#include <dagir/ir.hpp>
#include <dagir/ir_attrs.hpp>
#include <functional>
#include <iterator>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace dagir {

namespace render_expr_detail {

enum : int { PREC_OR = 1, PREC_AND = 2, PREC_NOT = 3, PREC_ATOM = 4 };

struct Expr {
  std::string s;
  int prec;
};

static std::string parenthesize_if_needed(const Expr& child, int parent_prec) {
  if (child.prec < parent_prec) return std::string("(") + child.s + ")";
  return child.s;
}

}  // namespace render_expr_detail

/**
 * @brief Render `ir_graph` as a boolean expression to `os`.
 *
 * This is a header-only helper similar in style to the other renderer
 * helpers in the project. It performs a memoized traversal of the graph
 * nodes and emits an expression equivalent to the BDD semantics.
 */
inline void render_expr(std::ostream& os, const ir_graph& g) {
  using namespace render_expr_detail;

  // Build node lookup for fast access
  std::unordered_map<std::uint64_t, const ir_node*> node_map;
  node_map.reserve(g.nodes.size());
  for (const auto& n : g.nodes) node_map.emplace(n.id, &n);

  // Build adjacency mapping from node id -> (false_target, true_target)
  struct Targets {
    std::uint64_t false_t = UINT64_MAX;
    std::uint64_t true_t = UINT64_MAX;
  };
  std::unordered_map<std::uint64_t, Targets> targets;
  targets.reserve(g.nodes.size());

  for (const auto& e : g.edges) {
    auto it = e.attributes.find(ir_attrs::k_style);
    std::string_view style = (it != e.attributes.end()) ? it->second : std::string_view{};
    if (style == std::string_view("dashed")) {
      targets[e.source].false_t = e.target;
    } else if (style == std::string_view("solid")) {
      targets[e.source].true_t = e.target;
    }
  }

  // Memoize computed expressions for node ids
  std::unordered_map<std::uint64_t, Expr> memo;

  // Recursive lambda to compute expression for node id
  std::function<Expr(std::uint64_t)> emit = [&](std::uint64_t id) -> Expr {
    auto mit = memo.find(id);
    if (mit != memo.end()) return mit->second;

    // Find node
    auto nit = node_map.find(id);
    if (nit == node_map.end()) {
      // Unknown node - treat as constant 0 for safety
      return memo.emplace(id, Expr{"0", PREC_ATOM}).first->second;
    }
    const ir_node& node = *nit->second;

    // Extract label (variable name or terminal value)
    std::string_view label;
    auto lit = node.attributes.find(ir_attrs::k_label);
    if (lit != node.attributes.end())
      label = lit->second;
    else
      label = std::string_view{};

    // Terminal nodes are labelled "0" or "1"
    if (label == std::string_view("0") || label == std::string_view("1")) {
      Expr e{std::string(label), PREC_ATOM};
      memo.emplace(id, e);
      return e;
    }

    // Non-terminal: find true/false children
    Targets t;
    auto tit = targets.find(id);
    if (tit != targets.end()) t = tit->second;

    Expr low = (t.false_t == UINT64_MAX) ? Expr{"0", PREC_ATOM} : emit(t.false_t);
    Expr high = (t.true_t == UINT64_MAX) ? Expr{"0", PREC_ATOM} : emit(t.true_t);

    // If both branches are equal, return branch expression
    if (low.s == high.s) {
      memo.emplace(id, high);
      return high;
    }

    // If high==1 and low==0 then result is variable
    std::string var_name = std::string(label);
    if (high.s == "1" && low.s == "0") {
      Expr e{var_name, PREC_ATOM};
      memo.emplace(id, e);
      return e;
    }
    if (high.s == "0" && low.s == "1") {
      // Equivalent to NOT var
      Expr e{std::string("NOT ") + var_name, PREC_NOT};
      memo.emplace(id, e);
      return e;
    }

    // General ITE -> (var AND high) OR (NOT var AND low)
    Expr var_expr{var_name, PREC_ATOM};
    Expr not_var_expr{std::string("NOT ") + var_name, PREC_NOT};

    // Build left = var AND high
    std::string left_s = parenthesize_if_needed(var_expr, PREC_AND) + " AND " +
                         parenthesize_if_needed(high, PREC_AND);
    Expr left{left_s, PREC_AND};

    // Build right = NOT var AND low
    std::string right_s = parenthesize_if_needed(not_var_expr, PREC_AND) + " AND " +
                          parenthesize_if_needed(low, PREC_AND);
    Expr right{right_s, PREC_AND};

    // Combine: left OR right
    std::string out_s =
        parenthesize_if_needed(left, PREC_OR) + " OR " + parenthesize_if_needed(right, PREC_OR);
    Expr out{out_s, PREC_OR};

    // Cache and return
    memo.emplace(id, out);
    return out;
  };

  // If no roots are present, emit nothing
  if (g.nodes.empty()) return;

  // Find roots: nodes that are not targets of any edge (graph roots)
  std::unordered_map<std::uint64_t, int> incoming;
  for (const auto& n : g.nodes) incoming.emplace(n.id, 0);
  for (const auto& e : g.edges) {
    auto it = incoming.find(e.target);
    if (it != incoming.end()) it->second++;
  }

  std::vector<std::uint64_t> roots;
  for (const auto& p : incoming)
    if (p.second == 0) roots.push_back(p.first);

  // For BDDs produced by the helpers the first root is the diagram root.
  // If multiple roots are present join them with OR.
  std::vector<Expr> root_exprs;
  std::transform(roots.begin(), roots.end(), std::back_inserter(root_exprs), emit);

  if (root_exprs.empty()) return;

  // Combine multiple root expressions with OR
  std::ostringstream oss;
  for (size_t i = 0; i < root_exprs.size(); ++i) {
    if (i) oss << " OR ";
    // Parenthesize top-level parts if they are lower precedence than OR
    oss << parenthesize_if_needed(root_exprs[i], PREC_OR);
  }

  os << oss.str();
}

}  // namespace dagir
