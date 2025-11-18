// SPDX-License-Identifier: MIT
// Copyright (c) DagIR Contributors
//
// IR types for renderer-neutral intermediate representation.

#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace dagir {

/**
 * @brief Key/value attribute attached to nodes, edges, or the global graph.
 */
using ir_attr_map = std::unordered_map<std::string, std::string>;

/**
 * @brief A node in the renderer-neutral IR.
 */
struct ir_node {
  std::uint64_t id;
  // cppcheck-suppress unusedStructMember
  [[maybe_unused]] ir_attr_map attributes;  ///< Node-specific attributes.
};

/**
 * @brief An edge in the renderer-neutral IR.
 */
struct ir_edge {
  std::uint64_t source;
  std::uint64_t target;
  // cppcheck-suppress unusedStructMember
  [[maybe_unused]] ir_attr_map attributes;
};

/**
 * @brief Top-level intermediate representation produced from a DAG view.
 */
struct ir_graph {
  // cppcheck-suppress unusedStructMember
  [[maybe_unused]] std::vector<ir_node> nodes;
  // cppcheck-suppress unusedStructMember
  [[maybe_unused]] std::vector<ir_edge> edges;
  // cppcheck-suppress unusedStructMember
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
