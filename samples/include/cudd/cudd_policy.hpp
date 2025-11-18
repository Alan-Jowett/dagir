/**
 * @file cudd_policy.hpp
 * @brief Node and edge attribute policies for CUDD BDD samples.
 *
 * @details
 * Provides `cudd_node_attributor` and `cudd_edge_attributor` used by
 * the sample pipeline to convert CUDD BDD nodes and edges into renderer-neutral
 * IR attributes. False edges are styled as dashed and true edges as solid.
 *
 * @copyright
 * © DagIR Contributors. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cudd/cudd.h>

#include <dagir/concepts/edge_attributor.hpp>
#include <dagir/concepts/node_attributor.hpp>
#include <dagir/ir.hpp>
#include <dagir/ir_attrs.hpp>

#include "cudd_read_only_dag_view.hpp"

namespace dagir {
namespace utility {

struct cudd_node_attributor {
  using view_t = cudd_read_only_dag_view;

  dagir::ir_attr_map operator()(const typename cudd_read_only_dag_view::handle& h) const {
    dagir::ir_attr_map out;
    if (!h.ptr) return out;

    if (Cudd_IsConstant(h.ptr)) {
      // In CUDD constants are represented as (possibly) complemented pointers
      // to the logical-one node. Use the complement flag to determine value.
      const bool is_complement = Cudd_IsComplement(h.ptr);
      int val = is_complement ? 0 : 1;
      out.emplace(std::string{dagir::ir_attrs::k_label}, val ? std::string{"1"} : std::string{"0"});
      out.emplace(std::string{dagir::ir_attrs::k_shape}, std::string{"box"});
      out.emplace(std::string{dagir::ir_attrs::k_fill_color}, std::string{"lightgray"});
    } else {
      DdNode* base = Cudd_Regular(h.ptr);
      std::string label = std::to_string(Cudd_NodeReadIndex(base));
      out.emplace(std::string{dagir::ir_attrs::k_label}, label);
      out.emplace(std::string{dagir::ir_attrs::k_shape}, std::string{"circle"});
    }

    return out;
  }

  dagir::ir_attr_map operator()(const cudd_read_only_dag_view& view,
                                const typename cudd_read_only_dag_view::handle& h) const {
    dagir::ir_attr_map out = operator()(h);
    if (!h.ptr) return out;

    const auto* names = view.var_names();
    if (names && !Cudd_IsConstant(h.ptr)) {
      DdNode* base = Cudd_Regular(h.ptr);
      int idx = Cudd_NodeReadIndex(base);
      if (idx >= 0 && static_cast<size_t>(idx) < names->size()) {
        const std::string nm = (*names)[static_cast<size_t>(idx)];
        out[std::string{dagir::ir_attrs::k_label}] = nm;
        out.emplace(std::string{"name"}, nm);
      }
    }

    return out;
  }
};

struct cudd_edge_attributor {
  using handle = typename cudd_read_only_dag_view::handle;

  dagir::ir_attr_map operator()(const cudd_read_only_dag_view& /*view*/, const handle& parent,
                                const handle& child) const {
    dagir::ir_attr_map out;
    if (!parent.ptr) return out;

    const bool is_comp = Cudd_IsComplement(parent.ptr);
    DdNode* base = Cudd_Regular(parent.ptr);
    DdNode* then_child = Cudd_T(base);
    DdNode* else_child = Cudd_E(base);
    if (is_comp) {
      if (else_child) else_child = Cudd_Not(else_child);
      if (then_child) then_child = Cudd_Not(then_child);
    }

    if (else_child && else_child == child.ptr) {
      out.emplace(std::string{dagir::ir_attrs::k_style}, std::string{"dashed"});
    } else if (then_child && then_child == child.ptr) {
      out.emplace(std::string{dagir::ir_attrs::k_style}, std::string{"solid"});
    }

    return out;
  }
};

}  // namespace utility
}  // namespace dagir
