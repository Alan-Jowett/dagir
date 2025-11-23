/**
 * @file expression_policy.hpp
 * @brief Helper utility for parsing string binary expressions
 *
 * @details
 *  This file defines policies for building ir_graph from expression ASTs. It includes things like
 * setting the node label to AND, OR, NOT, XOR, or variable name. It also colorizes the nodes based
 * on operator type.
 *
 * @copyright
 * © DagIR Contributors. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <dagir/concepts/edge_attributor.hpp>
#include <dagir/concepts/node_attributor.hpp>
#include <dagir/ir.hpp>
#include <dagir/ir_attrs.hpp>
#include <dagir/node_id.hpp>
#include <dagir/string_view_cache.hpp>
#include <dagir/utility/expressions/expression_ast.hpp>
#include <dagir/utility/expressions/expression_read_only_dag_view.hpp>
#include <string>
#include <unordered_map>

namespace dagir {
namespace utility {
/**
 * @brief Node attributor for expression AST nodes.
 *
 * This functor models `dagir::concepts::node_attributor`. For performance
 * the view-aware overloads return a `std::vector<std::pair<std::string_view,std::string_view>>`.
 * Computed strings are stored in a `mutable string_view_cache` member so
 * returned `string_view`s are stable for the duration of the call. The
 * `build_ir` utility will still migrate keys/values into the graph's
 * `attr_cache`.
 */
struct expression_node_attributor {
  using view_t = expression_read_only_dag_view;  // forward declaration use-case
  mutable dagir::string_view_cache cache;

  // Use shared helper: dagir::utility::make_node_id(key)

  std::vector<std::pair<std::string_view, std::string_view>> operator()(
      const typename expression_read_only_dag_view::handle& h) const {
    std::vector<std::pair<std::string_view, std::string_view>> out;
    const std::string_view key_label(dagir::ir_attrs::k_label);
    const std::string_view key_fill(dagir::ir_attrs::k_fill_color);
    const std::string_view key_style(dagir::ir_attrs::k_style);
    if (!h.ptr) return out;

    if (auto v = std::get_if<my_variable>(h.ptr)) {
      std::string varname(v->variable_name);
      out.emplace_back(key_label, cache.cache_view(varname));
      out.emplace_back(key_fill, std::string_view("lightblue"));
    } else if (std::get_if<my_and>(h.ptr)) {
      out.emplace_back(key_label, std::string_view("AND"));
      out.emplace_back(key_fill, std::string_view("lightgreen"));
      out.emplace_back(key_style, std::string_view("filled"));
    } else if (std::get_if<my_or>(h.ptr)) {
      out.emplace_back(key_label, std::string_view("OR"));
      out.emplace_back(key_fill, std::string_view("lightcoral"));
      out.emplace_back(key_style, std::string_view("filled"));
    } else if (std::get_if<my_xor>(h.ptr)) {
      out.emplace_back(key_label, std::string_view("XOR"));
      out.emplace_back(key_fill, std::string_view("lightpink"));
      out.emplace_back(key_style, std::string_view("filled"));
    } else if (std::get_if<my_not>(h.ptr)) {
      out.emplace_back(key_label, std::string_view("NOT"));
      out.emplace_back(key_fill, std::string_view("yellow"));
      out.emplace_back(key_style, std::string_view("filled"));
    }

    // Always expose a unique `name` attribute so renderers can use stable
    // unique node ids while keeping the human-visible `label` untouched.
    const std::string id = dagir::utility::make_node_id(h.stable_key());
    out.emplace_back(dagir::ir_attrs::k_id, cache.cache_view(id));
    return out;
  }

  std::vector<std::pair<std::string_view, std::string_view>> operator()(
      const expression_read_only_dag_view& /*view*/,
      const typename expression_read_only_dag_view::handle& h) const {
    return operator()(h);
  }
};

/**
 * @brief Edge attribute policy for expression AST edges.
 *
 * This functor models `dagir::concepts::edge_attributor` and returns a
 * `std::vector<std::pair<std::string_view,std::string_view>>` containing attribute key/value pairs.
 * Attributes encode a simple color/style scheme derived from the parent node's operator type.
 */
struct expression_edge_attributor {
  using handle = typename expression_read_only_dag_view::handle;

  std::vector<std::pair<std::string_view, std::string_view>> operator()(
      const expression_read_only_dag_view& /*view*/, const handle& parent,
      const handle& child) const {
    std::vector<std::pair<std::string_view, std::string_view>> out;
    if (!parent.ptr) return out;
    // For binary operators, label edges left/right. Keep some styles
    // (e.g. bold for AND, dashed for NOT) but do not set edge colors.
    const std::string_view key_label = dagir::ir_attrs::k_label;
    if (auto p_and = std::get_if<my_and>(parent.ptr)) {
      if (p_and->left && p_and->left.get() == child.ptr)
        out.emplace_back(key_label, std::string_view("L"));
      else if (p_and->right && p_and->right.get() == child.ptr)
        out.emplace_back(key_label, std::string_view("R"));
    } else if (auto p_or = std::get_if<my_or>(parent.ptr)) {
      if (p_or->left && p_or->left.get() == child.ptr)
        out.emplace_back(key_label, std::string_view("L"));
      else if (p_or->right && p_or->right.get() == child.ptr)
        out.emplace_back(key_label, std::string_view("R"));
    } else if (auto p_xor = std::get_if<my_xor>(parent.ptr)) {
      if (p_xor->left && p_xor->left.get() == child.ptr)
        out.emplace_back(key_label, std::string_view("L"));
      else if (p_xor->right && p_xor->right.get() == child.ptr)
        out.emplace_back(key_label, std::string_view("R"));
    }
    return out;
  }
};

}  // namespace utility
}  // namespace dagir