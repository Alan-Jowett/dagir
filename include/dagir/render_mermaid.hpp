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
inline std::string escape_mermaid(const std::string_view s) {
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
  // Ensure consistent appearance on platforms (e.g. GitHub) that may
  // apply a dark theme to Mermaid renderings. Emit an init directive
  // to request the default (light) Mermaid theme so node fill/stroke
  // colours remain visible.
  os << "%%{ init: {\"theme\": \"default\"} }%%\n";

  // Determine direction: prefer graph-level k_rankdir if present
  std::string rankdir = "TB";
  if (g.global_attrs.count(ir_attrs::k_rankdir)) rankdir = g.global_attrs.at(ir_attrs::k_rankdir);

  // Mermaid requires `graph <dir>` where <dir> is TB, LR, etc.
  os << "graph " << rankdir << "\n";

  // Emit title if provided (deterministic: check sorted keys)
  if (!g.global_attrs.empty()) {
    std::vector<std::string_view> gkeys;
    gkeys.reserve(g.global_attrs.size());
    std::transform(g.global_attrs.begin(), g.global_attrs.end(), std::back_inserter(gkeys),
                   [](auto const& p) { return p.first; });
    std::sort(gkeys.begin(), gkeys.end());
    bool found_title = false;
    for (const auto& k : gkeys) {
      if (k == ir_attrs::k_graph_label) {
        os << "  title " << render_mermaid_detail::escape_mermaid(g.global_attrs.at(k)) << "\n";
        found_title = true;
      }
    }
    if (found_title) {
      os << "%% " << render_mermaid_detail::escape_mermaid(graph_name) << "\n";
    }
  }

  // Emit nodes. Mermaid syntax for a node with a box is: n1[Label]
  for (const auto& n : g.nodes) {
    const auto& amap = n.attributes;

    // Determine label: prefer k_label, then id
    std::string label_gen;
    std::string_view label_sv;
    if (amap.count(ir_attrs::k_label)) {
      label_sv = amap.at(ir_attrs::k_label);
    } else {
      label_gen = std::format("{}", n.id);
      label_sv = label_gen;
    }

    // Determine shape: map some known shapes to Mermaid bracket syntax
    std::string opening = "[";
    std::string closing = "]";
    if (amap.count(ir_attrs::k_shape)) {
      const auto& s = amap.at(ir_attrs::k_shape);
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
    std::string node_name_gen;
    std::string_view node_name_sv;
    if (amap.count("name")) {
      node_name_sv = amap.at("name");
    } else {
      node_name_gen = std::format("n{}", n.id);
      node_name_sv = node_name_gen;
    }
    os << "  " << node_name_sv << opening << '"' << render_mermaid_detail::escape_mermaid(label_sv)
       << '"' << closing << "\n";

    // Emit simple style directive if fill or stroke is provided
    if (amap.count(ir_attrs::k_fill_color) || amap.count(ir_attrs::k_color) ||
        amap.count(ir_attrs::k_pen_width)) {
      std::vector<std::string> parts;
      if (amap.count(ir_attrs::k_fill_color))
        parts.push_back(std::format("fill:{}", amap.at(ir_attrs::k_fill_color)));
      if (amap.count(ir_attrs::k_color))
        parts.push_back(std::format("stroke:{}", amap.at(ir_attrs::k_color)));
      if (amap.count(ir_attrs::k_pen_width))
        parts.push_back(std::format("stroke-width:{}", amap.at(ir_attrs::k_pen_width)));
      if (!parts.empty()) {
        std::sort(parts.begin(), parts.end());
        os << "  style " << node_name_sv << " " << parts[0];
        for (size_t i = 1; i < parts.size(); ++i) os << "," << parts[i];
        os << "\n";
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
        if (a.count("name")) return std::string(a.at("name"));
        return std::format("n{}", it->id);
      }
      return std::format("n{}", nid);
    };

    const std::string src = find_node_name(e.source);
    const std::string dst = find_node_name(e.target);
    const auto& amap = e.attributes;
    if (amap.count(ir_attrs::k_label)) {
      os << "  " << src << " -- \""
         << render_mermaid_detail::escape_mermaid(amap.at(ir_attrs::k_label)) << "\" --> " << dst
         << "\n";
    } else {
      os << "  " << src << " --> " << dst << "\n";
    }
  }
}

}  // namespace dagir
