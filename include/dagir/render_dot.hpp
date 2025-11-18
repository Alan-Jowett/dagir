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
inline void write_attrs(std::ostream& os, const std::vector<ir_attr>& attrs) {
  bool first = true;
  for (const auto& a : attrs) {
    if (!first) os << ", ";
    first = false;
    // Emit key="escaped value"
    os << a.key << "=\"" << escape_dot(a.value) << "\"";
  }
}

/**
 * @brief Convert an attribute vector into a lookup map.
 *
 * This convenience helper is used by the simple emitter logic in this file
 * to perform presence checks and indexed lookups. It is intentionally
 * straightforward and trades a small amount of work for code clarity.
 */
inline std::unordered_map<std::string, std::string> attrs_to_map(
    const std::vector<ir_attr>& attrs) {
  std::unordered_map<std::string, std::string> m;
  for (const auto& a : attrs) m.emplace(a.key, a.value);
  return m;
}

}  // namespace render_dot_detail

// Writes a GraphViz DOT representation of `g` to `os`.
// `graph_name` is used as the DOT graph identifier.
inline void render_dot(std::ostream& os, const ir_graph& g, std::string_view graph_name = "G") {
  os << "digraph " << graph_name << " {\n";

  // Emit default rankdir only if the graph-level attributes do not provide one.
  const bool has_rankdir =
      std::any_of(g.global_attrs.begin(), g.global_attrs.end(),
                  [](const ir_attr& a) { return a.key == std::string(ir_attrs::k_rankdir); });
  if (!has_rankdir) {
    os << "  rankdir=TB;\n";  // default top-to-bottom layout
  }

  // First, emit global graph attributes (map known keys)
  for (const auto& a : g.global_attrs) {
    if (a.key == std::string(ir_attrs::k_graph_label)) {
      os << "  label=\"" << render_dot_detail::escape_dot(a.value) << "\";\n";
    } else {
      os << "  " << a.key << "=\"" << render_dot_detail::escape_dot(a.value) << "\";\n";
    }
  }

  std::unordered_map<std::uint64_t, std::string> name_map;
  // Gather node names for use in edge emission.

  // Emit nodes
  for (const auto& n : g.nodes) {
    const std::string node_name = !n.name.empty() ? n.name : std::format("n{}", n.id);

    name_map[n.id] = node_name;

    // Build attribute map from node.attrs and label fields
    auto amap = render_dot_detail::attrs_to_map(n.attributes);

    // Ensure label: prefer k_label, then node.label, then ir_node::name, then id
    std::string label;
    if (amap.count(std::string(ir_attrs::k_label)))
      label = amap[std::string(ir_attrs::k_label)];
    else if (!n.label.empty())
      label = n.label;
    else if (!n.name.empty())
      label = n.name;
    else
      label = std::format("{}", n.id);

    // Ensure nodes have a style of 'filled' by default
    if (!amap.count(std::string(ir_attrs::k_style))) {
      amap[std::string(ir_attrs::k_style)] = "filled";
    }

    // If no explicit fill color provided, choose a default based on label
    if (!amap.count(std::string(ir_attrs::k_fill_color))) {
      if (label == "AND")
        amap[std::string(ir_attrs::k_fill_color)] = "lightgreen";
      else if (label == "OR")
        amap[std::string(ir_attrs::k_fill_color)] = "lightcoral";
      else if (label == "XOR")
        amap[std::string(ir_attrs::k_fill_color)] = "lightpink";
      else if (label == "NOT")
        amap[std::string(ir_attrs::k_fill_color)] = "yellow";
      else
        amap[std::string(ir_attrs::k_fill_color)] = "lightblue";
    }

    os << "  " << node_name << " [";
    // Emit label first with spaces around '='
    os << "label = \"" << render_dot_detail::escape_dot(label) << "\"";

    // Emit other attributes, but skip keys we've handled
    for (const auto& kv : amap) {
      if (kv.first == std::string(ir_attrs::k_label)) continue;
      os << ", " << kv.first << " = \"" << render_dot_detail::escape_dot(kv.second) << "\"";
    }

    os << "];\n";
  }

  // Emit edges (use previously computed name_map for identifiers)
  for (const auto& e : g.edges) {
    const std::string src = name_map.at(e.source);
    const std::string dst = name_map.at(e.target);

    auto amap = render_dot_detail::attrs_to_map(e.attributes);

    os << "  " << src << " -> " << dst;
    if (!amap.empty()) {
      os << " [";
      bool first = true;
      // Prefer label from k_label
      if (amap.count(std::string(ir_attrs::k_label))) {
        os << "label = \"" << render_dot_detail::escape_dot(amap[std::string(ir_attrs::k_label)])
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
