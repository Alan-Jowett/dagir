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
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "dagir/ir_attrs.hpp"
#include "dagir/string_view_cache.hpp"

namespace dagir {

/**
 * @brief Non-owning view map for attributes.
 *
 * This map stores attribute keys and values as `std::string_view`. It does
 * not own the underlying character data — callers must ensure that any
 * `std::string_view` inserted into an `ir_attr_map` refers to storage that
 * remains alive for the lifetime of the `ir_graph` that contains it. The
 * intended usage pattern is for callers to prepare `std::string` keys/values
 * (for example via `std::unordered_map<std::string,std::string>`) and then
 * have `build_ir` cache those strings into `ir_graph::attr_cache` which
 * returns stable `std::string_view`s suitable for storing in `ir_attr_map`.
 *
 * Alternatives:
 * - Use `ir_graph::attr_cache.cache_view(...)` to obtain stable views for
 *   keys/values copied from temporary `std::string`s.
 * - Construct attributes as `std::unordered_map<std::string,std::string>` and
 *   let callers convert them via the builder helpers that populate the
 *   `attr_cache` (see `build_ir`).
 */
using ir_attr_map = std::unordered_map<std::string_view, std::string_view>;

/**
 * @brief A node in the renderer-neutral IR.
 *
 * `ir_node` holds a numeric identifier and a map of renderer-neutral
 * attributes. Attributes are stored as `std::string_view` and must refer to
 * storage that remains alive for the lifetime of the containing `ir_graph`.
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

/**
 * @brief Compare two nodes for deterministic ordering.
 *
 * Nodes are compared by their `name` attribute when present, falling back
 * to numeric id ordering for deterministic iteration order.
 */
inline bool operator<(ir_node const& a, ir_node const& b) {
  const auto a_it = a.attributes.find(ir_attrs::k_name);
  const auto b_it = b.attributes.find(ir_attrs::k_name);
  const bool a_has = (a_it != a.attributes.end());
  const bool b_has = (b_it != b.attributes.end());
  if (a_has && b_has) {
    const std::string_view a_name = a_it->second;
    const std::string_view b_name = b_it->second;
    if (a_name != b_name) return a_name < b_name;
    return a.id < b.id;
  }
  if (a_has && !b_has) return true;   // named nodes come before unnamed
  if (!a_has && b_has) return false;  // unnamed after named
  return a.id < b.id;
}

/**
 * @brief An edge in the renderer-neutral IR.
 *
 * `ir_edge` stores numeric source/target ids and an attribute map similar to
 * `ir_node`.
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

  /**
   * @brief Map of renderer-neutral attributes for the edge.
   *
   * Typical keys include `dagir::ir_attrs::k_label` for an edge label and
   * `dagir::ir_attrs::k_style` for visual styling hints.
   */
  [[maybe_unused]] ir_attr_map attributes;
};

/**
 * @brief Compare two edges for deterministic ordering.
 *
 * Edges are ordered by source, target and then by style attribute (if any)
 * to provide deterministic outputs from renderers.
 */
inline bool operator<(ir_edge const& a, ir_edge const& b) {
  // Compare by source id, then target id, then by style attribute (if present).
  const auto a_style_it = a.attributes.find(ir_attrs::k_style);
  const auto b_style_it = b.attributes.find(ir_attrs::k_style);
  const std::string_view a_style =
      (a_style_it != a.attributes.end()) ? a_style_it->second : std::string_view{};
  const std::string_view b_style =
      (b_style_it != b.attributes.end()) ? b_style_it->second : std::string_view{};
  return std::tie(a.source, a.target, a_style) < std::tie(b.source, b.target, b_style);
}

/**
 * @brief Top-level intermediate representation produced from a DAG view.
 *
 * `ir_graph` contains nodes, edges, and global attributes together with a
 * `string_view_cache` used to ensure attribute string views remain valid.
 */
struct ir_graph {
  /**
   * @brief All nodes present in the graph.
   *
   * Renderers use this vector to enumerate node identifiers and attributes.
   */
  [[maybe_unused]] std::vector<ir_node> nodes;

  /**
   * @brief All directed edges in the graph.
   *
   * Each edge references nodes via `source` and `target` numeric ids.
   */
  [[maybe_unused]] std::vector<ir_edge> edges;

  /**
   * @brief Global graph-level attributes.
   *
   * Backends may map known keys (for example `dagir::ir_attrs::k_graph_label`)
   * to renderer-specific properties; arbitrary metadata may also be stored
   * here for downstream consumers.
   */
  [[maybe_unused]] ir_attr_map global_attrs;

  /**
   * @brief Optional ordered list of root node identifiers.
   *
   * The `roots` vector holds node id strings (typically names when present
   * or numeric string ids) as `std::string_view`. Strings inserted here must
   * refer to storage that remains alive for the lifetime of the graph; use
   * `ir_graph::attr_cache.cache_view(...)` when constructing roots from
   * temporary `std::string`s.
   */
  [[maybe_unused]] std::vector<std::string_view> roots;

  // Cache for attribute strings produced by policies. This ensures that
  // `std::string_view` keys/values stored in `ir_attr_map` remain valid for
  // the lifetime of the graph.
  [[maybe_unused]] dagir::string_view_cache attr_cache;
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
  (void)&ir_graph::roots;
}
}  // namespace dagir
