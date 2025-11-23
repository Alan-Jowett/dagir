/**
 * @file teddy_policy.hpp
 * @brief Node and edge attribute policies for TeDDy BDD.
 *
 * @details
 * Provides `teddy_node_attributor` and `teddy_edge_attributor` used by the
 * sample pipeline to convert TeDDy BDD nodes and edges into renderer-neutral
 * IR attributes. False edges are styled as dashed and true edges as solid.
 *
 * SPDX-License-Identifier: MIT
 * © DagIR Contributors. All rights reserved.
 */

#pragma once

#include <dagir/concepts/edge_attributor.hpp>
#include <dagir/concepts/node_attributor.hpp>
#include <dagir/ir.hpp>
#include <dagir/ir_attrs.hpp>
#include <dagir/node_id.hpp>
#include <dagir/string_view_cache.hpp>
#include <dagir/utility/teddy/teddy_read_only_dag_view.hpp>

namespace dagir {
namespace utility {

/**
 * @brief Node attributor for TeDDy nodes.
 *
 * Returns a `std::vector<std::pair<std::string_view,std::string_view>>` of
 * attributes. Computed strings (numeric labels, generated ids) are cached
 * in the `mutable string_view_cache` member so returned views remain valid
 * for the duration of the call. `build_ir` will copy/cache these into the
 * graph's `attr_cache`.
 */
struct teddy_node_attributor {
  using view_t = teddy_read_only_dag_view;  // forward declaration use-case
  mutable dagir::string_view_cache cache;

  // Node id helper is provided by shared helper: dagir::utility::make_node_id(key)

  /**
   * @brief Produce attributes for a single node handle.
   * @param h The node handle.
   * @return Map of attribute key/value pairs.
   */
  std::vector<std::pair<std::string_view, std::string_view>> operator()(
      const typename teddy_read_only_dag_view::handle& h) const {
    std::vector<std::pair<std::string_view, std::string_view>> out;
    const std::string_view key_label(dagir::ir_attrs::k_label);
    const std::string_view key_shape(dagir::ir_attrs::k_shape);
    const std::string_view key_fill(dagir::ir_attrs::k_fill_color);
    if (!h.ptr) return out;

    // Terminal nodes: show their boolean value
    if (h.ptr->is_terminal()) {
      // Ensure terminal nodes are labeled with either "0" or "1" explicitly.
      const auto val = h.ptr->get_value();
      out.emplace_back(key_label, val ? std::string_view("1") : std::string_view("0"));
      out.emplace_back(key_shape, std::string_view("box"));
      out.emplace_back(key_fill, std::string_view("lightgray"));
    } else {
      // Variable nodes: label with variable name if available, otherwise index
      std::string label = std::to_string(h.ptr->get_index());
      // view-aware overload will populate var_names via the view argument when available
      auto lbl_sv = cache.cache_view(label);
      out.emplace_back(key_label, lbl_sv);
      out.emplace_back(key_shape, std::string_view("circle"));
    }

    return out;
  }

  /**
   * @brief Two-argument overload that forwards to the single-argument form.
   */
  std::vector<std::pair<std::string_view, std::string_view>> operator()(
      const teddy_read_only_dag_view& view,
      const typename teddy_read_only_dag_view::handle& h) const {
    auto out = operator()(h);
    if (!h.ptr) return out;

    // If the view provided variable names, use them for variable nodes
    const auto* names = view.var_names();
    if (names && !h.ptr->is_terminal()) {
      int idx = h.ptr->get_index();
      if (idx >= 0 && static_cast<size_t>(idx) < names->size()) {
        std::string_view nm = (*names)[static_cast<size_t>(idx)];
        const std::string_view key_label(dagir::ir_attrs::k_label);
        auto it = std::find_if(out.begin(), out.end(),
                               [&](const auto& pair) { return pair.first == key_label; });
        if (it != out.end()) it->second = nm;
      }
    }

    // Always assign a unique renderer-visible id attribute derived from the
    // node's stable key. This ensures distinct nodes receive distinct ids
    // even when labels collide.
    const std::string id = dagir::utility::make_node_id(h.stable_key());
    out.emplace_back(dagir::ir_attrs::k_id, cache.cache_view(id));
    return out;
  }
};

/**
 * @brief Edge attribute policy for TeDDy BDD edges.
 *
 * Returns lightweight view-pairs for style hints (e.g. dashed/solid) using
 * string literals or `ir_attrs` keys to avoid allocations in hot paths.
 * Styles the false branch (son 0) as dashed and the true branch (son 1)
 * as solid.
 */
struct teddy_edge_attributor {
  using handle = typename teddy_read_only_dag_view::handle;

  /**
   * @brief Produce attributes for an edge from `parent` to `child`.
   * @param view The view (unused).
   * @param parent Parent node handle.
   * @param child Child node handle.
   * @return Map of attribute key/value pairs.
   */
  std::vector<std::pair<std::string_view, std::string_view>> operator()(
      const teddy_read_only_dag_view& /*view*/, const handle& parent, const handle& child) const {
    std::vector<std::pair<std::string_view, std::string_view>> out;
    if (!parent.ptr) return out;

    // Determine whether this child is the false (0) or true (1) branch.
    auto son0 = parent.ptr->get_son(0);
    auto son1 = parent.ptr->get_son(1);

    if (son0 && son0 == child.ptr) {
      out.emplace_back(dagir::ir_attrs::k_style, std::string_view("dashed"));
    } else if (son1 && son1 == child.ptr) {
      out.emplace_back(dagir::ir_attrs::k_style, std::string_view("solid"));
    }

    return out;
  }
};

}  // namespace utility
}  // namespace dagir
