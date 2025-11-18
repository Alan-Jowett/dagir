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
#include <dagir/concepts/node_labeler.hpp>
#include <dagir/ir.hpp>
#include <dagir/ir_attrs.hpp>
#include <dagir/utility/expression_ast.hpp>
#include <dagir/utility/expression_read_only_dag_view.hpp>

namespace dagir {
namespace utility {
/**
 * @brief Node label policy for expression AST nodes.
 *
 * This functor models `dagir::concepts::node_labeler`. It supports both
 * `label(handle)` and `label(view, handle)` invocation forms by forwarding
 * the two-argument call to the single-argument implementation.
 */
struct expression_node_labeler {
  using view_t = expression_read_only_dag_view;  // forward declaration use-case

  std::string operator()(const typename expression_read_only_dag_view::handle& h) const {
    if (!h.ptr) return std::string{};

    if (auto v = std::get_if<my_variable>(h.ptr)) {
      return v->variable_name;
    } else if (std::get_if<my_and>(h.ptr)) {
      return std::string{"AND"};
    } else if (std::get_if<my_or>(h.ptr)) {
      return std::string{"OR"};
    } else if (std::get_if<my_xor>(h.ptr)) {
      return std::string{"XOR"};
    } else if (std::get_if<my_not>(h.ptr)) {
      return std::string{"NOT"};
    }

    return std::string{};
  }

  std::string operator()(const expression_read_only_dag_view& /*view*/,
                         const typename expression_read_only_dag_view::handle& h) const {
    return operator()(h);
  }
};

/**
 * @brief Edge attribute policy for expression AST edges.
 *
 * This functor models `dagir::concepts::edge_attributor` and returns a
 * small vector of `dagir::ir_attr` entries. Attributes encode a simple
 * color/style scheme derived from the parent node's operator type.
 */
struct expression_edge_attributor {
  using handle = typename expression_read_only_dag_view::handle;

  std::vector<dagir::ir_attr> operator()(const expression_read_only_dag_view& /*view*/,
                                         const handle& parent, const handle& child) const {
    std::vector<dagir::ir_attr> out;
    if (!parent.ptr) return out;

    auto add = [&out](std::string key, std::string value) {
      dagir::ir_attr a;
      a.key = std::move(key);
      a.value = std::move(value);
      out.push_back(std::move(a));
    };
    // For binary operators, label edges left/right. Keep some styles
    // (e.g. bold for AND, dashed for NOT) but do not set edge colors.
    if (auto p_and = std::get_if<my_and>(parent.ptr)) {
      if (p_and->left && p_and->left.get() == child.ptr)
        add(std::string{dagir::ir_attrs::k_label}, std::string{"L"});
      else if (p_and->right && p_and->right.get() == child.ptr)
        add(std::string{dagir::ir_attrs::k_label}, std::string{"R"});
    } else if (auto p_or = std::get_if<my_or>(parent.ptr)) {
      if (p_or->left && p_or->left.get() == child.ptr)
        add(std::string{dagir::ir_attrs::k_label}, std::string{"L"});
      else if (p_or->right && p_or->right.get() == child.ptr)
        add(std::string{dagir::ir_attrs::k_label}, std::string{"R"});
    } else if (auto p_xor = std::get_if<my_xor>(parent.ptr)) {
      if (p_xor->left && p_xor->left.get() == child.ptr)
        add(std::string{dagir::ir_attrs::k_label}, std::string{"L"});
      else if (p_xor->right && p_xor->right.get() == child.ptr)
        add(std::string{dagir::ir_attrs::k_label}, std::string{"R"});
    } else if (auto p_not = std::get_if<my_not>(parent.ptr)) {
      // Unary operator: do not emit edge labels for NOT -- leave edges unlabeled
      (void)child;
    } else if (std::get_if<my_variable>(parent.ptr)) {
      // variables have no outgoing labeled edges
    }

    return out;
  }
};

}  // namespace utility
}  // namespace dagir