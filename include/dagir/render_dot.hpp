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
#include <iomanip>
#include <ostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace dagir {

namespace detail {

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
          char buf[5];
          std::snprintf(buf, sizeof(buf), "\\x%02x", c);
          out += buf;
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

}  // namespace detail

// Writes a GraphViz DOT representation of `g` to `os`.
// `graph_name` is used as the DOT graph identifier.
inline void render_dot(std::ostream& os, const ir_graph& g, std::string_view graph_name = "G") {
  os << "digraph " << graph_name << " {\n";

  // Emit default rankdir only if the graph-level attributes do not provide one.
  bool has_rankdir = false;
  for (const auto& a : g.global_attrs) {
    if (a.key == std::string(ir_attrs::k_rankdir)) {
      has_rankdir = true;
      break;
    }
  }
  if (!has_rankdir) {
    os << "  rankdir=TB;\n";  // default top-to-bottom layout
  }

  // First, emit global graph attributes (map known keys)
  for (const auto& a : g.global_attrs) {
    if (a.key == std::string(ir_attrs::k_graph_label)) {
      os << "  label=\"" << detail::escape_dot(a.value) << "\";\n";
    } else {
      os << "  " << a.key << "=\"" << detail::escape_dot(a.value) << "\";\n";
    }
  }

  // Emit nodes
  for (const auto& n : g.nodes) {
    const std::string node_name = "n" + std::to_string(n.id);

    // Build attribute map from node.attrs and label fields
    auto amap = detail::attrs_to_map(n.attributes);

    // Ensure label: prefer k_label, then node.label, then id
    std::string label;
    if (amap.count(std::string(ir_attrs::k_label)))
      label = amap[std::string(ir_attrs::k_label)];
    else if (!n.label.empty())
      label = n.label;
    else
      label = std::to_string(n.id);

    // If fill color present, advise style=filled unless style set
    if (amap.count(std::string(ir_attrs::k_fill_color)) &&
        !amap.count(std::string(ir_attrs::k_style))) {
      amap[std::string(ir_attrs::k_style)] = "filled";
    }

    os << "  " << node_name << " [";
    // Emit label first
    os << "label=\"" << detail::escape_dot(label) << "\"";

    // Emit other attributes, but skip keys we've handled
    for (const auto& kv : amap) {
      if (kv.first == std::string(ir_attrs::k_label)) continue;
      os << ", " << kv.first << "=\"" << detail::escape_dot(kv.second) << "\"";
    }

    os << "];\n";
  }

  // Emit edges
  for (const auto& e : g.edges) {
    const std::string src = "n" + std::to_string(e.source);
    const std::string dst = "n" + std::to_string(e.target);

    auto amap = detail::attrs_to_map(e.attributes);

    os << "  " << src << " -> " << dst;
    if (!amap.empty()) {
      os << " [";
      bool first = true;
      // Prefer label from k_label
      if (amap.count(std::string(ir_attrs::k_label))) {
        os << "label=\"" << detail::escape_dot(amap[std::string(ir_attrs::k_label)]) << "\"";
        first = false;
      }
      for (const auto& kv : amap) {
        if (kv.first == std::string(ir_attrs::k_label)) continue;
        if (!first) os << ", ";
        first = false;
        os << kv.first << "=\"" << detail::escape_dot(kv.second) << "\"";
      }
      os << "]";
    }
    os << ";\n";
  }

  os << "}\n";
}

}  // namespace dagir
