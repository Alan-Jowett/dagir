/**
 * @file
 * @brief Header-only Mermaid renderer for `dagir::ir_graph`.
 *
 * This header emits a Mermaid `graph` representation of a `dagir::ir_graph`
 * to an output stream. It honours a subset of `dagir::ir_attrs` where it makes
 * sense for Mermaid (labels, graph title, simple node fill/stroke styles,
 * and rank direction via `rankdir`). The implementation is intentionally
 * small and conservative so it can be used in tools and tests without
 * additional dependencies.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <algorithm>
#include <dagir/ir.hpp>
#include <dagir/ir_attrs.hpp>
#include <format>
#include <iterator>
#include <numeric>
#include <ostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace dagir {

namespace render_mermaid_detail {

/**
 * @brief Escape a string for inclusion in Mermaid labels.
 *
 * This performs conservative escaping of control characters and quotes so
 * labels are safe to include inside Mermaid quoted labels.
 */
inline std::string escape_mermaid(const std::string& s) {
  std::string out;
  out.reserve(s.size() + 8);
  for (unsigned char c : s) {
    switch (c) {
      case '\\':
        out += "\\\\";
        break;
      case '"':
        out += "\"";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      default:
        if (c < 0x20) {
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
 * @brief Convert IR attribute vector to a lookup map.
 *
 * Convenience helper to simplify attribute lookup when emitting nodes/edges.
 */
// Attributes are now stored as `ir_attr_map`; helper removed.

}  // namespace render_mermaid_detail

/**
 * @brief Render `ir_graph` as a Mermaid `graph` to `os`.
 *
 * @param os Output stream to write Mermaid syntax to.
 * @param g The intermediate representation to render.
 * @param graph_name Optional identifier for the graph (used in comments only).
 */
inline void render_mermaid(std::ostream& os, const ir_graph& g, std::string_view graph_name = "G") {
  // Determine direction: prefer graph-level k_rankdir if present
  std::string rankdir = "TB";
  if (g.global_attrs.count(std::string(ir_attrs::k_rankdir)))
    rankdir = g.global_attrs.at(std::string(ir_attrs::k_rankdir));

  // Mermaid requires `graph <dir>` where <dir> is TB, LR, etc.
  os << "graph " << rankdir << "\n";

  // Emit title if provided
  for (const auto& kv : g.global_attrs) {
    if (kv.first == std::string(ir_attrs::k_graph_label)) {
      os << "  title " << render_mermaid_detail::escape_mermaid(kv.second) << "\n";
    }
  }

  // Emit nodes. Mermaid syntax for a node with a box is: n1[Label]
  for (const auto& n : g.nodes) {
    const auto& amap = n.attributes;

    // Determine label: prefer k_label, then id
    std::string label = amap.count(std::string(ir_attrs::k_label))
                            ? amap.at(std::string(ir_attrs::k_label))
                            : std::format("{}", n.id);

    // Determine shape: map some known shapes to Mermaid bracket syntax
    std::string opening = "[";
    std::string closing = "]";
    if (amap.count(std::string(ir_attrs::k_shape))) {
      const auto& s = amap.at(std::string(ir_attrs::k_shape));
      if (s == "circle" || s == "ellipse") {
        opening = "(";
        closing = ")";
      } else if (s == "round" || s == "stadium") {
        opening = "((";
        closing = "))";
      } else if (s == "diamond") {
        opening = "<>";  // Mermaid does not support diamond directly; fall back
        closing = "<>";
      }
    }

    // Prefer attribute "name" for the identifier used in edges and styles.
    std::string node_name = amap.count("name") ? amap.at("name") : std::format("n{}", n.id);
    os << "  " << node_name << opening << '"' << render_mermaid_detail::escape_mermaid(label) << '"'
       << closing << "\n";

    // Emit simple style directive if fill or stroke is provided
    if (amap.count(std::string(ir_attrs::k_fill_color)) ||
        amap.count(std::string(ir_attrs::k_color)) ||
        amap.count(std::string(ir_attrs::k_pen_width))) {
      std::vector<std::string> parts;
      if (amap.count(std::string(ir_attrs::k_fill_color)))
        parts.push_back(std::format("fill:{}", amap.at(std::string(ir_attrs::k_fill_color))));
      if (amap.count(std::string(ir_attrs::k_color)))
        parts.push_back(std::format("stroke:{}", amap.at(std::string(ir_attrs::k_color))));
      if (amap.count(std::string(ir_attrs::k_pen_width)))
        parts.push_back(
            std::format("stroke-width:{}", amap.at(std::string(ir_attrs::k_pen_width))));
      if (!parts.empty()) {
        os << "  style " << node_name << " "
           << std::format("{}", std::accumulate(std::next(parts.begin()), parts.end(), parts[0],
                                                [](const std::string& a, const std::string& b) {
                                                  return a + "," + b;
                                                }))
           << "\n";
      }
    }
  }

  // Emit edges. Mermaid edge label syntax: A -- "label" --> B
  for (const auto& e : g.edges) {
    auto find_node_name = [&](std::uint64_t nid) -> std::string {
      auto it = std::find_if(g.nodes.begin(), g.nodes.end(),
                             [&](const ir_node& nn) { return nn.id == nid; });
      if (it != g.nodes.end()) {
        const auto& a = it->attributes;
        if (a.count("name")) return a.at("name");
        return std::format("n{}", it->id);
      }
      return std::format("n{}", nid);
    };

    const std::string src = find_node_name(e.source);
    const std::string dst = find_node_name(e.target);
    const auto& amap = e.attributes;
    if (amap.count(std::string(ir_attrs::k_label))) {
      os << "  " << src << " -- \""
         << render_mermaid_detail::escape_mermaid(amap.at(std::string(ir_attrs::k_label)))
         << "\" --> " << dst << "\n";
    } else {
      os << "  " << src << " --> " << dst << "\n";
    }
  }
}

}  // namespace dagir
