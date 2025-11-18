/**
 * @file
 * @brief Header-only GraphViz DOT renderer for `dagir::ir_graph`.
 *
 * This header provides a minimal, header-only helper to format a
 * `dagir::ir_graph` as a GraphViz DOT `digraph` into an output stream.
 * It honors a small set of `dagir::ir_attrs` where applicable (labels,
 * colors, shapes, style hints) and performs conservative escaping of
 * attribute values so the produced DOT is syntactically valid.
 *
 * The implementation is intentionally small and header-only so it can be
 * used by tests and tools without introducing additional build-time
 * dependencies. Internal helpers live in the `detail` namespace; the
 * primary public entry point is `dagir::render_dot`.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <algorithm>
#include <dagir/ir.hpp>
#include <dagir/ir_attrs.hpp>
#include <format>
#include <functional>
#include <iomanip>
#include <ostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace dagir {

namespace render_dot_detail {

/**
 * @brief Escape a string for inclusion inside a quoted GraphViz attribute.
 *
 * This routine performs conservative escaping for characters that are
 * significant inside DOT quoted strings (backslash, double quotes, newlines,
 * etc.) and emits hex escapes for other non-printable control characters.
 * It is intentionally conservative to avoid producing DOT that the parser
 * might misinterpret.
 */
inline std::string escape_dot(const std::string& s) {
  std::string out;
  out.reserve(s.size() + 8);
  for (size_t i = 0; i < s.size(); ++i) {
    unsigned char c = static_cast<unsigned char>(s[i]);
    switch (c) {
      case '\\':
        // Always escape backslash
        out += "\\\\";
        // If this backslash is followed by an escString letter, preserve the
        // literal by leaving the following character as-is (we've already
        // escaped the backslash itself).
        break;
      case '"':
        out += "\\\"";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      case '\f':
        out += "\\f";
        break;
      case '\v':
        out += "\\v";
        break;
      default:
        if (c < 0x20) {
          // Non-printable control characters: emit hex escape to avoid
          // introducing invalid bytes in DOT source.
          out += std::format("\\x{:02x}", static_cast<unsigned>(c));
        } else {
          out += static_cast<char>(c);
        }
        break;
    }
  }
  return out;
}

/**
 * @brief Helper that emits a comma-separated list of attributes to `os`.
 *
 * Each attribute is emitted as `key="escaped value"`. This is an internal
 * helper used during node/edge emission.
 */
/**
 * @brief Convert an attribute vector into a lookup map.
 *
 * This convenience helper is used by the simple emitter logic in this file
 * to perform presence checks and indexed lookups. It is intentionally
 * straightforward and trades a small amount of work for code clarity.
 */
// Attributes are stored as `ir_attr_map` in the IR; helpers are not needed.

}  // namespace render_dot_detail

// Writes a GraphViz DOT representation of `g` to `os`.
// `graph_name` is used as the DOT graph identifier.
inline void render_dot(std::ostream& os, const ir_graph& g, std::string_view graph_name = "G") {
  os << "digraph " << graph_name << " {\n";

  // Emit default rankdir only if the graph-level attributes do not provide one.
  if (!g.global_attrs.count(std::string(ir_attrs::k_rankdir))) {
    os << "  rankdir=TB;\n";  // default top-to-bottom layout
  }

  // First, emit global graph attributes (map known keys)
  for (const auto& kv : g.global_attrs) {
    if (kv.first == std::string(ir_attrs::k_graph_label)) {
      os << "  label=\"" << render_dot_detail::escape_dot(kv.second) << "\";\n";
    } else {
      os << "  " << kv.first << "=\"" << render_dot_detail::escape_dot(kv.second) << "\";\n";
    }
  }

  std::unordered_map<std::uint64_t, std::string> name_map;
  // Gather node names for use in edge emission.

  // Emit nodes
  for (const auto& n : g.nodes) {
    const std::string node_name = !n.name.empty() ? n.name : std::format("n{}", n.id);

    name_map[n.id] = node_name;

    // Attribute map from node (now stored directly in `ir_node::attributes`)
    const auto& amap = n.attributes;

    // Ensure label: prefer k_label, then node.label, then ir_node::name, then id
    std::string label;
    if (amap.count(std::string(ir_attrs::k_label)))
      label = amap.at(std::string(ir_attrs::k_label));
    else if (!n.label.empty())
      label = n.label;
    else if (!n.name.empty())
      label = n.name;
    else
      label = std::format("{}", n.id);

    // Work from a local mutable copy when applying defaults so we don't mutate the
    // const attribute map stored on the node.
    auto local = amap;
    if (!local.count(std::string(ir_attrs::k_style))) {
      local[std::string(ir_attrs::k_style)] = "filled";
    }

    if (!local.count(std::string(ir_attrs::k_fill_color))) {
      if (label == "AND")
        local[std::string(ir_attrs::k_fill_color)] = "lightgreen";
      else if (label == "OR")
        local[std::string(ir_attrs::k_fill_color)] = "lightcoral";
      else if (label == "XOR")
        local[std::string(ir_attrs::k_fill_color)] = "lightpink";
      else if (label == "NOT")
        local[std::string(ir_attrs::k_fill_color)] = "yellow";
      else
        local[std::string(ir_attrs::k_fill_color)] = "lightblue";
    }

    // Emit node using the possibly-updated local map.
    os << "  " << node_name << " [";
    os << "label = \"" << render_dot_detail::escape_dot(label) << "\"";
    for (const auto& kv : local) {
      if (kv.first == std::string(ir_attrs::k_label)) continue;
      os << ", " << kv.first << " = \"" << render_dot_detail::escape_dot(kv.second) << "\"";
    }
    os << "];\n";
  }

  // Emit edges (use previously computed name_map for identifiers)
  for (const auto& e : g.edges) {
    const std::string src = name_map.at(e.source);
    const std::string dst = name_map.at(e.target);

    const auto& amap = e.attributes;

    os << "  " << src << " -> " << dst;
    if (!amap.empty()) {
      os << " [";
      bool first = true;
      if (amap.count(std::string(ir_attrs::k_label))) {
        os << "label = \"" << render_dot_detail::escape_dot(amap.at(std::string(ir_attrs::k_label)))
           << "\"";
        first = false;
      }
      for (const auto& kv : amap) {
        if (kv.first == std::string(ir_attrs::k_label)) continue;
        if (!first) os << ", ";
        first = false;
        os << kv.first << " = \"" << render_dot_detail::escape_dot(kv.second) << "\"";
      }
      os << "]";
    }
    os << ";\n";
  }

  os << "}\n";
}

}  // namespace dagir
