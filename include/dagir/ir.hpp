// SPDX-License-Identifier: MIT
// Copyright (c) DagIR Contributors
//
// IR types for renderer-neutral intermediate representation.

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace dagir {

/**
 * @brief Key/value attribute attached to nodes, edges, or the global graph.
 */
struct IRAttr {
  // The attributes are intentionally present for consumers and may be
  // unused in some translation units. Suppress cppcheck's unused-struct
  // member style warning for these fields.
  // cppcheck-suppress unusedStructMember
  [[maybe_unused]] std::string key;
  // cppcheck-suppress unusedStructMember
  [[maybe_unused]] std::string value;
};

/**
 * @brief A node in the renderer-neutral IR.
 */
struct IRNode {
  std::uint64_t id;
  // cppcheck-suppress unusedStructMember
  [[maybe_unused]] std::string label;
  // cppcheck-suppress unusedStructMember
  [[maybe_unused]] std::vector<IRAttr> attributes;
};

/**
 * @brief An edge in the renderer-neutral IR.
 */
struct IREdge {
  std::uint64_t source;
  std::uint64_t target;
  // cppcheck-suppress unusedStructMember
  [[maybe_unused]] std::vector<IRAttr> attributes;
};

/**
 * @brief Top-level intermediate representation produced from a DAG view.
 */
struct IRGraph {
  // cppcheck-suppress unusedStructMember
  [[maybe_unused]] std::vector<IRNode> nodes;
  // cppcheck-suppress unusedStructMember
  [[maybe_unused]] std::vector<IREdge> edges;
  // cppcheck-suppress unusedStructMember
  [[maybe_unused]] std::vector<IRAttr> global_attrs;
};

// Touch pointer-to-members for fields that may be unused in some TUs.
// This provides a compile-time usage pattern that satisfies static
// analyzers without impacting runtime behaviour.
inline void __dagir_touch_ir_members() {
  (void)&IRAttr::key;
  (void)&IRAttr::value;
  (void)&IRNode::label;
  (void)&IRNode::attributes;
  (void)&IREdge::attributes;
  (void)&IRGraph::nodes;
  (void)&IRGraph::edges;
  (void)&IRGraph::global_attrs;
}
}  // namespace dagir
