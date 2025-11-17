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
    std::string key;
    std::string value;
};

/**
 * @brief A node in the renderer-neutral IR.
 */
struct IRNode {
    std::uint64_t id;
    std::string label;
    std::vector<IRAttr> attributes;
};

/**
 * @brief An edge in the renderer-neutral IR.
 */
struct IREdge {
    std::uint64_t source;
    std::uint64_t target;
    std::vector<IRAttr> attributes;
};

/**
 * @brief Top-level intermediate representation produced from a DAG view.
 */
struct IRGraph {
    std::vector<IRNode> nodes;
    std::vector<IREdge> edges;
    std::vector<IRAttr> global_attrs;
};

} // namespace dagir
