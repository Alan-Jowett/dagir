/**
 * @file cudd_policy.hpp
 * @brief Node and edge attribute policies for CUDD BDD.
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
#include <dagir/node_id.hpp>
#include <dagir/string_view_cache.hpp>
#include <dagir/utility/cudd/cudd_read_only_dag_view.hpp>

namespace dagir {
namespace utility {
/**
 * @brief Node attributor for CUDD nodes.
 *
 * Produces renderer-neutral attributes for CUDD nodes. The view-aware
 * overloads return a lightweight sequence of key/value pairs as
 * `std::vector<std::pair<std::string_view,std::string_view>>`.
 * Returned string_views must refer to storage that outlives the call; this
 * implementation stores any computed strings (numeric labels, generated
 * ids) in a `mutable string_view_cache` member so callers may safely return
 * views into that storage. The outer `build_ir` will still cache all
 * returned keys/values into the graph's `attr_cache`.
 */
struct cudd_node_attributor {
  using view_t = cudd_read_only_dag_view;

  // Cache for generated strings so returned string_views remain valid
  mutable dagir::string_view_cache cache;

  // Return vector of key/value string_views (views may point to literals,
  // ir_attrs keys, view-owned strings, or cached strings stored in `cache`).
  std::vector<std::pair<std::string_view, std::string_view>> operator()(
      const typename cudd_read_only_dag_view::handle& h) const {
    std::vector<std::pair<std::string_view, std::string_view>> out;
    const std::string_view key_label = dagir::ir_attrs::k_label;
    const std::string_view key_shape = dagir::ir_attrs::k_shape;
    const std::string_view key_fill = dagir::ir_attrs::k_fill_color;
    if (!h.ptr) return out;

    if (Cudd_IsConstant(h.ptr)) {
      // In CUDD constants are represented as (possibly) complemented pointers
      // to the logical-one node. Use the complement flag to determine value.
      const bool is_complement = Cudd_IsComplement(h.ptr);
      out.emplace_back(key_label, is_complement ? std::string_view("0") : std::string_view("1"));
      out.emplace_back(key_shape, std::string_view("box"));
      out.emplace_back(key_fill, std::string_view("lightgray"));
    } else {
      DdNode* base = Cudd_Regular(h.ptr);
      std::string lbl = std::to_string(Cudd_NodeReadIndex(base));
      auto lbl_sv = cache.cache_view(lbl);
      out.emplace_back(key_label, lbl_sv);
      out.emplace_back(key_shape, std::string_view("circle"));
    }

    return out;
  }

  std::vector<std::pair<std::string_view, std::string_view>> operator()(
      const cudd_read_only_dag_view& view,
      const typename cudd_read_only_dag_view::handle& h) const {
    auto out = operator()(h);
    if (!h.ptr) return out;

    const auto* names = view.var_names();
    if (names && !Cudd_IsConstant(h.ptr)) {
      DdNode* base = Cudd_Regular(h.ptr);
      int idx = Cudd_NodeReadIndex(base);
      if (idx >= 0 && static_cast<size_t>(idx) < names->size()) {
        std::string_view nm_sv = (*names)[static_cast<size_t>(idx)];
        // Replace/add label entry
        const std::string_view key_label = dagir::ir_attrs::k_label;
        // Replace existing label entry if present; prefer std::find_if
        auto it = std::find_if(out.begin(), out.end(),
                               [&](const auto& pair) { return pair.first == key_label; });
        if (it != out.end()) {
          it->second = nm_sv;
        } else {
          out.emplace_back(key_label, nm_sv);
        }
      }
    }

    // Assign unique node id attribute based on stable key
    const std::string id = dagir::utility::make_node_id(h.stable_key());
    auto id_sv = cache.cache_view(id);
    out.emplace_back(dagir::ir_attrs::k_id, id_sv);

    return out;
  }
};

/**
 * @brief Edge attributor for CUDD edges.
 *
 * Produces a small vector of attribute key/value string_views for each
 * edge. Style values are returned as string literals when possible.
 */
struct cudd_edge_attributor {
  using handle = typename cudd_read_only_dag_view::handle;

  std::vector<std::pair<std::string_view, std::string_view>> operator()(
      const cudd_read_only_dag_view& /*view*/, const handle& parent, const handle& child) const {
    std::vector<std::pair<std::string_view, std::string_view>> out;
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
      out.emplace_back(dagir::ir_attrs::k_style, std::string_view("dashed"));
    } else if (then_child && then_child == child.ptr) {
      out.emplace_back(dagir::ir_attrs::k_style, std::string_view("solid"));
    }

    return out;
  }
};

}  // namespace utility
}  // namespace dagir
