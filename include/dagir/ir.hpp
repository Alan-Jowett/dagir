/**
 * @file ir.hpp
 * @brief Renderer-neutral intermediate representation types (nodes, edges, graph).
 *
 * SPDX-License-Identifier: MIT
 * Copyright (c) DagIR Contributors
 */

#pragma once

#include <cstdint>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "dagir/ir_attrs.hpp"

namespace dagir {

/**
 * @brief Key/value attribute attached to nodes, edges, or the global graph.
 */
using ir_attr_map = std::unordered_map<std::string, std::string>;

/**
 * @brief A node in the renderer-neutral IR.
 */
struct ir_node {
  /**
   * @brief Numeric identifier for this node.
   *
   * This id is used by renderers and consumers as a stable numeric handle
   * for building edges and for deterministic ordering when no explicit
   * `name` attribute is present.
   */
  std::uint64_t id;

  // cppcheck-suppress unusedStructMember
  /**
   * @brief Map of renderer-neutral attributes for the node.
   *
   * Keys should generally be chosen from `dagir::ir_attrs` (for example
   * `dagir::ir_attrs::k_label`) but arbitrary string keys are allowed for
   * downstream consumers.
   */
  [[maybe_unused]] ir_attr_map attributes;  ///< Node-specific attributes.
};

inline bool operator<(ir_node const& a, ir_node const& b) {
  const auto a_it = a.attributes.find(std::string{"name"});
  const auto b_it = b.attributes.find(std::string{"name"});
  const bool a_has = (a_it != a.attributes.end());
  const bool b_has = (b_it != b.attributes.end());
  if (a_has && b_has) {
    const std::string& a_name = a_it->second;
    const std::string& b_name = b_it->second;
    if (a_name != b_name) return a_name < b_name;
    return a.id < b.id;
  }
  if (a_has && !b_has) return true;   // named nodes come before unnamed
  if (!a_has && b_has) return false;  // unnamed after named
  return a.id < b.id;
}

/**
 * @brief An edge in the renderer-neutral IR.
 */
struct ir_edge {
  /**
   * @brief Numeric id of the source node.
   *
   * This references an `ir_node::id` value in the graph's `nodes` vector.
   */
  std::uint64_t source;

  /**
   * @brief Numeric id of the target (destination) node.
   *
   * This references an `ir_node::id` value in the graph's `nodes` vector.
   */
  std::uint64_t target;

  // cppcheck-suppress unusedStructMember
  /**
   * @brief Map of renderer-neutral attributes for the edge.
   *
   * Typical keys include `dagir::ir_attrs::k_label` for an edge label and
   * `dagir::ir_attrs::k_style` for visual styling hints.
   */
  [[maybe_unused]] ir_attr_map attributes;
};

inline bool operator<(ir_edge const& a, ir_edge const& b) {
  // Compare by source id, then target id, then by style attribute (if present).
  const auto a_style_it = a.attributes.find(std::string{ir_attrs::k_style});
  const auto b_style_it = b.attributes.find(std::string{ir_attrs::k_style});
  const std::string a_style =
      (a_style_it != a.attributes.end()) ? a_style_it->second : std::string{};
  const std::string b_style =
      (b_style_it != b.attributes.end()) ? b_style_it->second : std::string{};
  return std::tie(a.source, a.target, a_style) < std::tie(b.source, b.target, b_style);
}

/**
 * @brief Top-level intermediate representation produced from a DAG view.
 */
struct ir_graph {
  // cppcheck-suppress unusedStructMember
  /**
   * @brief All nodes present in the graph.
   *
   * Renderers use this vector to enumerate node identifiers and attributes.
   */
  [[maybe_unused]] std::vector<ir_node> nodes;

  // cppcheck-suppress unusedStructMember
  /**
   * @brief All directed edges in the graph.
   *
   * Each edge references nodes via `source` and `target` numeric ids.
   */
  [[maybe_unused]] std::vector<ir_edge> edges;

  // cppcheck-suppress unusedStructMember
  /**
   * @brief Global graph-level attributes.
   *
   * Backends may map known keys (for example `dagir::ir_attrs::k_graph_label`)
   * to renderer-specific properties; arbitrary metadata may also be stored
   * here for downstream consumers.
   */
  [[maybe_unused]] ir_attr_map global_attrs;
};

// Touch pointer-to-members for fields that may be unused in some TUs.
// This provides a compile-time usage pattern that satisfies static
// analyzers without impacting runtime behaviour.
// Touch pointer-to-members for fields that may be unused in some TUs.
// This provides a compile-time usage pattern that satisfies static
// analyzers without impacting runtime behaviour.
inline void touch_ir_members_for_static_analysis() {
  (void)sizeof(ir_attr_map);
  (void)&ir_node::attributes;
  (void)&ir_edge::attributes;
  (void)&ir_graph::nodes;
  (void)&ir_graph::edges;
  (void)&ir_graph::global_attrs;
}
}  // namespace dagir
