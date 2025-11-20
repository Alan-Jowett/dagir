/**
 * @file
 * @brief Header-only SVG renderer for `dagir::ir_graph`.
 *
 * This is a minimal, dependency-free SVG emitter intended for tests and
 * simple tooling. It places nodes on a rectangular grid and draws straight
 * line edges. It maps a small subset of `dagir::ir_attrs` to SVG styling
 * attributes (fill, stroke, stroke-width, font).
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <algorithm>
#include <cmath>
#include <dagir/ir.hpp>
#include <dagir/ir_attrs.hpp>
#include <format>
#include <ostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace dagir {

namespace render_svg_detail {

inline std::string escape_xml(std::string_view s) {
  std::string out;
  out.reserve(s.size());
  for (unsigned char c : s) {
    switch (c) {
      case '&':
        out += "&amp;";
        break;
      case '<':
        out += "&lt;";
        break;
      case '>':
        out += "&gt;";
        break;
      case '"':
        out += "&quot;";
        break;
      case '\'':
        out += "&apos;";
        break;
      default:
        out += static_cast<char>(c);
        break;
    }
  }
  return out;
}

inline std::string lookup_or(const ir_attr_map& m, std::string_view k, std::string_view def = "") {
  auto it = m.find(std::string{k});
  if (it == m.end()) return std::string(def);
  return it->second;
}

}  // namespace render_svg_detail

/**
 * @brief Render `ir_graph` to a simple SVG document.
 */
inline void render_svg(std::ostream& os, const ir_graph& g, std::string_view title = "DagIR") {
  // Layout constants
  const double node_w = 140.0;
  const double node_h = 40.0;
  const double h_gap = 40.0;
  const double v_gap = 28.0;
  const double margin = 10.0;

  const size_t N = g.nodes.size();
  const size_t cols =
      std::max<size_t>(1, static_cast<size_t>(std::floor(std::sqrt(static_cast<double>(N)))));
  const size_t rows = (N + cols - 1) / cols;

  const double svg_w = margin * 2 + cols * node_w + (cols - 1) * h_gap;
  const double svg_h = margin * 2 + rows * node_h + (rows - 1) * v_gap + 30.0;  // space for title

  os << std::format(
      "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"{}\" height=\"{}\" viewBox=\"0 0 {} "
      "{}\">\n",
      static_cast<int>(std::ceil(svg_w)), static_cast<int>(std::ceil(svg_h)),
      static_cast<int>(std::ceil(svg_w)), static_cast<int>(std::ceil(svg_h)));

  // Title
  if (!g.global_attrs.empty()) {
    auto it = g.global_attrs.find(std::string(ir_attrs::k_graph_label));
    if (it != g.global_attrs.end()) {
      os << "  <text x=\"" << (svg_w / 2.0) << "\" y=\"16\" text-anchor=\"middle\">"
         << render_svg_detail::escape_xml(it->second) << "</text>\n";
    }
  } else {
    os << "  <text x=\"" << (svg_w / 2.0) << "\" y=\"16\" text-anchor=\"middle\">"
       << render_svg_detail::escape_xml(title) << "</text>\n";
  }

  // arrow marker
  os << "  <defs>\n";
  os << "    <marker id=\"dagir-arrow\" markerWidth=\"10\" markerHeight=\"10\" refX=\"10\" "
        "refY=\"5\" orient=\"auto\">\n";
  os << "      <path d=\"M0,0 L10,5 L0,10 z\" fill=\"black\" />\n";
  os << "    </marker>\n";
  os << "  </defs>\n";

  // Map node id -> centre position and element id
  std::unordered_map<std::uint64_t, std::pair<double, double>> centers;
  std::unordered_map<std::uint64_t, std::string> elem_id;

  for (size_t idx = 0; idx < g.nodes.size(); ++idx) {
    const auto& n = g.nodes[idx];
    const size_t r = idx / cols;
    const size_t c = idx % cols;
    const double x = margin + c * (node_w + h_gap);
    const double y = margin + r * (node_h + v_gap) + 24.0;  // offset for title
    const double cx = x + node_w / 2.0;
    const double cy = y + node_h / 2.0;
    centers[n.id] = {cx, cy};

    const auto& amap = n.attributes;
    const std::string name = amap.count("name") ? amap.at("name") : std::format("n{}", n.id);
    // produce a safe element id
    std::string id_safe = std::format("dagir-{}", n.id);
    elem_id[n.id] = id_safe;

    // style mapping
    const std::string fill =
        render_svg_detail::lookup_or(amap, std::string_view(ir_attrs::k_fill_color), "#ffffff");
    const std::string stroke =
        render_svg_detail::lookup_or(amap, std::string_view(ir_attrs::k_color), "#000000");
    const std::string penw =
        render_svg_detail::lookup_or(amap, std::string_view(ir_attrs::k_pen_width), "1");
    const std::string fontsize =
        render_svg_detail::lookup_or(amap, std::string_view(ir_attrs::k_font_size), "12");
    const std::string fontname =
        render_svg_detail::lookup_or(amap, std::string_view(ir_attrs::k_font_name), "sans-serif");

    // rect
    os << std::format("  <g id=\"{}\">\n", id_safe);
    os << std::format(
        "    <rect x=\"{:.1f}\" y=\"{:.1f}\" width=\"{:.1f}\" height=\"{:.1f}\" rx=\"6\" ry=\"6\" "
        "fill=\"{}\" stroke=\"{}\" stroke-width=\"{}\" />\n",
        x, y, node_w, node_h, render_svg_detail::escape_xml(fill),
        render_svg_detail::escape_xml(stroke), render_svg_detail::escape_xml(penw));

    // label
    const std::string label =
        amap.count(std::string(ir_attrs::k_label)) ? amap.at(std::string(ir_attrs::k_label)) : name;
    os << std::format(
        "    <text x=\"{:.1f}\" y=\"{:.1f}\" text-anchor=\"middle\" alignment-baseline=\"middle\" "
        "font-family=\"{}\" font-size=\"{}\">{}</text>\n",
        cx, cy, render_svg_detail::escape_xml(fontname), render_svg_detail::escape_xml(fontsize),
        render_svg_detail::escape_xml(label));
    os << "  </g>\n";
  }

  // Edges as straight lines
  for (const auto& e : g.edges) {
    const auto s_it = centers.find(e.source);
    const auto t_it = centers.find(e.target);
    if (s_it == centers.end() || t_it == centers.end()) continue;
    const double sx = s_it->second.first;
    const double sy = s_it->second.second;
    const double tx = t_it->second.first;
    const double ty = t_it->second.second;

    const auto& amap = e.attributes;
    const std::string stroke =
        render_svg_detail::lookup_or(amap, std::string_view(ir_attrs::k_color), "#000000");
    const std::string penw =
        render_svg_detail::lookup_or(amap, std::string_view(ir_attrs::k_pen_width), "1");

    os << std::format(
        "  <line x1=\"{:.1f}\" y1=\"{:.1f}\" x2=\"{:.1f}\" y2=\"{:.1f}\" stroke=\"{}\" "
        "stroke-width=\"{}\" marker-end=\"url(#dagir-arrow)\" />\n",
        sx, sy, tx, ty, render_svg_detail::escape_xml(stroke), render_svg_detail::escape_xml(penw));

    if (amap.count(std::string(ir_attrs::k_label))) {
      // place edge label at midpoint
      const double lx = (sx + tx) / 2.0;
      const double ly = (sy + ty) / 2.0 - 6.0;
      os << std::format(
          "  <text x=\"{:.1f}\" y=\"{:.1f}\" text-anchor=\"middle\" font-size=\"10\">{}</text>\n",
          lx, ly, render_svg_detail::escape_xml(amap.at(std::string(ir_attrs::k_label))));
    }
  }

  os << "</svg>\n";
}

}  // namespace dagir
/**
 * @file
 * @brief Header-only SVG DOT renderer for `dagir::ir_graph`.
 *
 * This header provides a minimal, header-only helper to format a
 * `dagir::ir_graph` as a SVG representation of DAG.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once
