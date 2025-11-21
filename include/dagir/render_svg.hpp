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
#include <cstdlib>
#include <dagir/ir.hpp>
#include <dagir/ir_attrs.hpp>
#include <dagir/sugiyama.hpp>
#include <format>
#include <limits>
#include <ostream>
#include <random>
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

/**
 * Shared renderer state and computed layout values used across helpers.
 * This aggregates configuration constants, intermediate structures and
 * derived values to avoid passing long parameter lists between helpers.
 */
struct RenderState {
  // layout constants
  double node_w = 70.0;
  double node_h = 36.0;
  double h_gap = 24.0;
  double v_gap = 28.0 * 1.75;
  double margin = 8.0;

  // derived SVG extents
  double svg_w = 0.0;
  double svg_h = 0.0;

  // drawing area helpers
  double area_w = 0.0;
  double area_h = 0.0;
  double available_w = 0.0;
  double available_h = 0.0;

  // helper precomputed values
  double half_node_w = 0.0;
  double half_node_h = 0.0;

  // rank metadata
  int min_rank = 0;
  int max_rank = 0;
  int levels = 1;
  bool has_reachable = false;

  // containers built during layout
  std::vector<std::uint64_t> ids;
  std::unordered_map<std::uint64_t, const ir_attr_map*> attr_map;
  std::unordered_map<std::uint64_t, std::pair<double, double>> pos;
  std::unordered_map<std::uint64_t, int> rank_val;
  std::unordered_map<int, double> rank_y;
  std::unordered_map<int, std::vector<std::uint64_t>> by_rank;
  std::vector<std::pair<std::uint64_t, std::uint64_t>> edges;
  std::unordered_map<std::uint64_t, std::string> elem_id;
};

/**
 * Create the initial RenderState: compute ids, attr_map, basic svg size,
 * ranks, random initial positions and adjacency list of edges.
 */
inline RenderState create_initial_state(const ir_graph& g, std::string_view /*title*/) {
  RenderState st;
  const size_t N = g.nodes.size();
  const size_t cols =
      std::max<size_t>(1, static_cast<size_t>(std::floor(std::sqrt(static_cast<double>(N)))));
  const size_t rows = (N + cols - 1) / cols;

  st.half_node_w = st.node_w / 2.0;
  st.half_node_h = st.node_h / 2.0;

  st.svg_w = st.margin * 2 + cols * st.node_w + (cols - 1) * st.h_gap + st.node_w;
  st.svg_h = st.margin * 2 + rows * st.node_h + (rows - 1) * st.v_gap + 30.0 + st.node_h;

  st.area_w = st.svg_w - 2.0 * st.margin;
  st.area_h = st.svg_h - 2.0 * st.margin - 24.0;
  st.available_w = std::max(1.0, st.area_w - 2.0 * st.half_node_w);
  st.available_h = std::max(1.0, st.area_h - 2.0 * st.half_node_h);

  st.ids.reserve(g.nodes.size());
  std::transform(g.nodes.begin(), g.nodes.end(), std::back_inserter(st.ids),
                 [](const auto& n) { return n.id; });

  for (const auto& n : g.nodes) st.attr_map[n.id] = &n.attributes;

  std::mt19937_64 rng(1234567);
  std::uniform_real_distribution<double> ux(st.margin + st.half_node_w,
                                            st.margin + st.half_node_w + st.available_w);
  std::uniform_real_distribution<double> uy(st.margin + 24.0 + st.half_node_h,
                                            st.margin + 24.0 + st.half_node_h + st.available_h);

  // ranks
  int min_rank = std::numeric_limits<int>::max();
  int max_rank = std::numeric_limits<int>::min();
  bool has_reachable = false;
  for (auto id : st.ids) {
    int r = -1;
    if (st.attr_map.count(id)) {
      const auto& amap = *st.attr_map[id];
      try {
        r = std::stoi(lookup_or(amap, std::string_view(ir_attrs::k_rank), "-1"));
      } catch (...) {
        r = -1;
      }
    }
    st.rank_val[id] = r;
    if (r >= 0) {
      has_reachable = true;
      min_rank = std::min(min_rank, r);
      max_rank = std::max(max_rank, r);
    }
  }

  int levels = 1;
  if (has_reachable) levels = max_rank - min_rank + 1;
  const bool has_unreachable = std::any_of(st.ids.begin(), st.ids.end(),
                                           [&](std::uint64_t id) { return st.rank_val[id] < 0; });
  if (has_unreachable) ++levels;
  st.min_rank = min_rank;
  st.max_rank = max_rank;
  st.levels = levels;
  st.has_reachable = has_reachable;

  const double min_rank_v_step = st.node_h * (4.0 / 3.0);
  if (levels > 1) {
    const double required_v_span = min_rank_v_step * static_cast<double>(levels - 1);
    const double current_v_span = st.available_h;
    if (required_v_span > current_v_span) {
      const double extra_v = required_v_span - current_v_span;
      st.svg_h += extra_v;
      const double new_area_h = st.svg_h - 2.0 * st.margin - 24.0;
      const double new_available_h = std::max(1.0, new_area_h - 2.0 * st.half_node_h);
      st.available_h = new_available_h;
    }
  }

  // rank_y
  if (levels == 1) {
    st.rank_y[0] = st.margin + 24.0 + st.half_node_h + st.available_h / 2.0;
  } else {
    for (int i = 0; i < levels; ++i) {
      double tpos = static_cast<double>(i) / static_cast<double>(levels - 1);
      st.rank_y[i] = st.margin + 24.0 + st.half_node_h + tpos * st.available_h;
    }
  }

  // initial positions
  for (auto id : st.ids) {
    int r = st.rank_val[id];
    int idx = 0;
    if (r < 0)
      idx = st.levels - 1;
    else if (st.has_reachable)
      idx = r - st.min_rank;
    else
      idx = 0;
    st.pos[id] = {ux(rng), st.rank_y[idx]};
  }

  // edges
  st.edges.reserve(g.edges.size());
  std::transform(g.edges.begin(), g.edges.end(), std::back_inserter(st.edges),
                 [](const auto& e) { return std::make_pair(e.source, e.target); });

  for (const auto& n : g.nodes) st.elem_id[n.id] = std::format("dagir-{}", n.id);

  return st;
}

/**
 * Resolve overlaps within each rank, apply barycenter reordering and
 * transposition passes. Produces final `centers` map of node id -> x,y.
 */
inline std::unordered_map<std::uint64_t, std::pair<double, double>> resolve_overlaps_and_ordering(
    RenderState& st, const ir_graph& g) {
  // Build by_rank grouping
  for (auto id : st.ids) {
    int r = st.rank_val[id];
    int idx = 0;
    if (r < 0)
      idx = st.levels - 1;
    else if (st.has_reachable)
      idx = r - st.min_rank;
    else
      idx = 0;
    st.by_rank[idx].push_back(id);
  }

  // adjacency
  std::unordered_map<std::uint64_t, std::vector<std::uint64_t>> outgoing, incoming;
  for (const auto& e : st.edges) {
    outgoing[e.first].push_back(e.second);
    incoming[e.second].push_back(e.first);
  }

  auto rank_index_of = [&](std::uint64_t id) -> int {
    int r = st.rank_val[id];
    if (r < 0) return st.levels - 1;
    if (!st.has_reachable) return 0;
    return r - st.min_rank;
  };

  const int reorder_passes = 8;
  for (int pass = 0; pass < reorder_passes; ++pass) {
    for (int r = 1; r < static_cast<int>(st.rank_y.size()); ++r) {
      auto& vec = st.by_rank[r];
      if (vec.size() <= 1) continue;
      std::vector<std::pair<double, std::uint64_t>> order;
      order.reserve(vec.size());
      for (auto id : vec) {
        std::vector<double> xs;
        xs.reserve(incoming[id].size());
        for (auto nb : incoming[id])
          if (rank_index_of(nb) == r - 1) xs.push_back(st.pos[nb].first);
        double key;
        if (xs.empty())
          key = st.pos[id].first;
        else {
          std::sort(xs.begin(), xs.end());
          const size_t m = xs.size();
          if (m % 2 == 1)
            key = xs[m / 2];
          else
            key = 0.5 * (xs[m / 2 - 1] + xs[m / 2]);
        }
        order.emplace_back(key, id);
      }
      std::stable_sort(order.begin(), order.end(),
                       [](const auto& a, const auto& b) { return a.first < b.first; });
      for (size_t i = 0; i < order.size(); ++i) vec[i] = order[i].second;
    }

    for (int r = static_cast<int>(st.rank_y.size()) - 2; r >= 0; --r) {
      auto& vec = st.by_rank[r];
      if (vec.size() <= 1) continue;
      std::vector<std::pair<double, std::uint64_t>> order;
      order.reserve(vec.size());
      for (auto id : vec) {
        std::vector<double> xs;
        xs.reserve(outgoing[id].size());
        for (auto nb : outgoing[id])
          if (rank_index_of(nb) == r + 1) xs.push_back(st.pos[nb].first);
        double key;
        if (xs.empty())
          key = st.pos[id].first;
        else {
          std::sort(xs.begin(), xs.end());
          const size_t m = xs.size();
          if (m % 2 == 1)
            key = xs[m / 2];
          else
            key = 0.5 * (xs[m / 2 - 1] + xs[m / 2]);
        }
        order.emplace_back(key, id);
      }
      std::stable_sort(order.begin(), order.end(),
                       [](const auto& a, const auto& b) { return a.first < b.first; });
      for (size_t i = 0; i < order.size(); ++i) vec[i] = order[i].second;
    }
  }

  auto count_crossings_between = [&](const std::vector<std::uint64_t>& left,
                                     const std::vector<std::uint64_t>& right) -> size_t {
    std::unordered_map<std::uint64_t, size_t> pos_right;
    for (size_t i = 0; i < right.size(); ++i) pos_right[right[i]] = i;
    std::vector<size_t> targets;
    targets.reserve(left.size());
    for (auto u : left)
      for (auto v : outgoing[u]) {
        auto it = pos_right.find(v);
        if (it != pos_right.end()) targets.push_back(it->second);
      }
    size_t inv = 0;
    for (size_t i = 0; i < targets.size(); ++i) {
      inv += std::count_if(targets.begin() + i + 1, targets.end(),
                           [&](size_t v) { return targets[i] > v; });
    }
    return inv;
  };

  const int transpose_max_iters = 6;
  for (int titer = 0; titer < transpose_max_iters; ++titer) {
    bool swapped_any = false;
    for (int r = 0; r < static_cast<int>(st.rank_y.size()); ++r) {
      auto& vec = st.by_rank[r];
      if (vec.size() <= 1) continue;
      std::vector<std::uint64_t> prev = (r > 0) ? st.by_rank[r - 1] : std::vector<std::uint64_t>();
      std::vector<std::uint64_t> next = (r + 1 < static_cast<int>(st.rank_y.size()))
                                            ? st.by_rank[r + 1]
                                            : std::vector<std::uint64_t>();
      for (size_t i = 0; i + 1 < vec.size(); ++i) {
        size_t before = 0;
        if (!prev.empty()) before += count_crossings_between(prev, vec);
        if (!next.empty()) before += count_crossings_between(vec, next);
        std::swap(vec[i], vec[i + 1]);
        size_t after = 0;
        if (!prev.empty()) after += count_crossings_between(prev, vec);
        if (!next.empty()) after += count_crossings_between(vec, next);
        if (after < before)
          swapped_any = true;
        else
          std::swap(vec[i], vec[i + 1]);
      }
    }
    if (!swapped_any) break;
  }

  const double left_center = st.margin + st.half_node_w;
  const double right_center = st.margin + st.half_node_w + st.available_w;
  for (auto& kv : st.by_rank) {
    auto& vec = kv.second;
    if (vec.empty()) continue;
    std::sort(vec.begin(), vec.end(),
              [&](std::uint64_t a, std::uint64_t b) { return st.pos[a].first < st.pos[b].first; });
    const std::size_t m = vec.size();
    if (m == 1) {
      st.pos[vec[0]].first = std::clamp(st.pos[vec[0]].first, left_center, right_center);
    } else {
      const double span = right_center - left_center;
      const double min_step = st.node_w * (4.0 / 3.0);
      double step = span / static_cast<double>(m - 1);
      if (span >= min_step * static_cast<double>(m - 1)) {
        step = min_step;
        double block_width = step * static_cast<double>(m - 1);
        double start = left_center + (span - block_width) / 2.0;
        for (std::size_t i = 0; i < m; ++i)
          st.pos[vec[i]].first = start + step * static_cast<double>(i);
      } else {
        for (std::size_t i = 0; i < m; ++i)
          st.pos[vec[i]].first = left_center + step * static_cast<double>(i);
      }
    }
  }

  // centers from pos with Y anchored to rank_y
  std::unordered_map<std::uint64_t, std::pair<double, double>> centers = st.pos;
  for (auto& kv : centers) {
    int r = st.rank_val[kv.first];
    int ridx = 0;
    if (r < 0)
      ridx = st.levels - 1;
    else if (st.has_reachable)
      ridx = r - st.min_rank;
    else
      ridx = 0;
    kv.second.second = st.rank_y[ridx];
  }

  return centers;
}

/**
 * Integrate Sugiyama layout. This updates `centers` and may
 * expand `svg_w`/`available_w` to ensure minimum per-rank spacing.
 */
inline void apply_sugiyama_layout(
    const ir_graph& g, RenderState& st,
    std::unordered_map<std::uint64_t, std::pair<double, double>>& centers) {
  const char* dagir_svg_layout_env = std::getenv("DAGIR_SVG_LAYOUT");
  bool use_sugiyama = true;
  if (dagir_svg_layout_env && std::string(dagir_svg_layout_env) == "classic") use_sugiyama = false;
  if (!use_sugiyama) return;

  dagir::sugiyama_options opts;
  opts.node_dist = st.h_gap;
  opts.layer_dist = st.v_gap;
  auto coords = dagir::sugiyama_layout_compute(g, opts);

  double sug_left_center = st.margin + st.half_node_w;
  double sug_right_center = st.margin + st.half_node_w + st.available_w;

  std::unordered_map<int, std::size_t> per_rank_count_sug;
  auto rank_index_of_index = [&](size_t idx) -> int {
    int r = -1;
    if (idx < g.nodes.size()) {
      const auto& amap = g.nodes[idx].attributes;
      try {
        r = std::stoi(lookup_or(amap, std::string_view(ir_attrs::k_rank), "-1"));
      } catch (...) {
        r = -1;
      }
    }
    if (r < 0) return st.levels - 1;
    if (!st.has_reachable) return 0;
    return r - st.min_rank;
  };
  for (size_t i = 0; i < coords.x.size() && i < g.nodes.size(); ++i) {
    int ridx = rank_index_of_index(i);
    per_rank_count_sug[ridx] += 1;
  }
  const std::size_t max_per_rank_sug =
      std::accumulate(per_rank_count_sug.begin(), per_rank_count_sug.end(), std::size_t{0},
                      [](std::size_t acc, const auto& kv) { return std::max(acc, kv.second); });
  const double min_center_step = st.node_w * (4.0 / 3.0);
  if (max_per_rank_sug > 1) {
    const double required_span = min_center_step * static_cast<double>(max_per_rank_sug - 1);
    const double current_span = sug_right_center - sug_left_center;
    if (required_span > current_span) {
      const double extra = required_span - current_span;
      st.svg_w += extra;
      st.area_w = st.svg_w - 2.0 * st.margin;
      st.available_w = std::max(1.0, st.area_w - 2.0 * st.half_node_w);
      sug_right_center = st.margin + st.half_node_w + st.available_w;
    }
  }

  std::unordered_map<int, std::vector<std::pair<double, size_t>>> sug_by_rank;
  for (size_t i = 0; i < coords.x.size() && i < g.nodes.size(); ++i) {
    int ridx = rank_index_of_index(i);
    sug_by_rank[ridx].emplace_back(coords.x[i], i);
  }

  const double min_step = st.node_w * (4.0 / 3.0);
  for (auto& kv : sug_by_rank) {
    int ridx = kv.first;
    auto& vec = kv.second;
    if (vec.empty()) continue;
    std::sort(vec.begin(), vec.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });
    const size_t m = vec.size();
    double span = sug_right_center - sug_left_center;
    double step = (m <= 1) ? 0.0 : (span / static_cast<double>(m - 1));
    bool enforce_min = (span >= min_step * static_cast<double>(m - 1));
    double start;
    if (m == 1) {
      start = sug_left_center + span * 0.5;
      centers[g.nodes[vec[0].second].id] = {start, st.rank_y[ridx]};
    } else {
      if (enforce_min) {
        step = min_step;
        double block_width = step * static_cast<double>(m - 1);
        start = sug_left_center + (span - block_width) / 2.0;
      } else {
        start = sug_left_center;
      }
      for (size_t i = 0; i < m; ++i) {
        double nx = start + step * static_cast<double>(i);
        centers[g.nodes[vec[i].second].id] = {nx, st.rank_y[ridx]};
      }
    }
  }
}

/**
 * Emit the final SVG output (header, defs, edges, nodes) using finalized
 * `centers` and current RenderState values. This mirrors the original
 * emission code but is extracted for clarity.
 */
inline void emit_svg_output(
    std::ostream& os, const ir_graph& g, RenderState& st,
    const std::unordered_map<std::uint64_t, std::pair<double, double>>& centers,
    std::string_view title) {
  // Helper: build marker id map and marker list from edge styles
  auto build_markers = [&](const ir_graph& graph,
                          std::unordered_map<std::string, std::string>& out_map,
                          std::vector<std::pair<std::string, std::string>>& out_markers) {
    int marker_ctr = 0;
    for (const auto& e : graph.edges) {
      const auto& amap = e.attributes;
      const std::string stroke = lookup_or(amap, std::string_view(ir_attrs::k_color), "#000000");
      const std::string penw = lookup_or(amap, std::string_view(ir_attrs::k_pen_width), "1");
      const std::string key = stroke + "|" + penw;
      if (out_map.find(key) == out_map.end()) {
        const std::string id = std::format("dagir-arrow-{}", marker_ctr++);
        out_map[key] = id;
        out_markers.emplace_back(id, stroke);
      }
    }
  };

  // Helper: render edges (writes base lines, collects marker segments)
  auto render_edges = [&](std::ostream& os_inner,
                          const ir_graph& graph,
                          const RenderState& state,
                          const std::unordered_map<std::uint64_t, std::pair<double, double>>& ctrs,
                          const std::unordered_map<std::string, std::string>& marker_id_for_style)
      -> std::vector<std::string> {
    std::vector<std::string> marker_segments;
    for (const auto& e : graph.edges) {
      const auto s_it = ctrs.find(e.source);
      const auto t_it = ctrs.find(e.target);
      if (s_it == ctrs.end() || t_it == ctrs.end()) continue;
      double sx = s_it->second.first;
      double sy = s_it->second.second;
      double tx = t_it->second.first;
      double ty = t_it->second.second;

      const auto& amap = e.attributes;
      const std::string stroke = lookup_or(amap, std::string_view(ir_attrs::k_color), "#000000");
      const std::string penw = lookup_or(amap, std::string_view(ir_attrs::k_pen_width), "1");

      const double dx = tx - sx;
      const double dy = ty - sy;
      double len = std::sqrt(dx * dx + dy * dy);
      if (len < 1e-6) continue;
      const double nx = dx / len;
      const double ny = dy / len;
      const double half_x = state.node_w / 2.0;
      const double half_y = state.node_h / 2.0;
      auto compute_t_rect = [&](double nxv, double nyv, double hx, double hy) {
        const double epsv = 1e-9;
        const double ax = std::abs(nxv);
        const double ay = std::abs(nyv);
        if (ax < epsv && ay < epsv) return 0.0;
        if (ax < epsv) return hy / ay;
        if (ay < epsv) return hx / ax;
        return std::min(hx / ax, hy / ay);
      };
      auto compute_t_ellipse = [&](double nxv, double nyv, double rx, double ry) {
        const double eps = 1e-12;
        const double denom = std::sqrt((nxv * nxv) / (rx * rx) + (nyv * nyv) / (ry * ry));
        return (denom < eps) ? 0.0 : (1.0 / denom);
      };

        const auto& s_attrs =
          graph.nodes
            .at(std::distance(graph.nodes.begin(),
                    std::find_if(graph.nodes.begin(), graph.nodes.end(),
                           [&](const auto& n) { return n.id == e.source; })))
            .attributes;
        const auto& t_attrs =
          graph.nodes
            .at(std::distance(graph.nodes.begin(),
                    std::find_if(graph.nodes.begin(), graph.nodes.end(),
                           [&](const auto& n) { return n.id == e.target; })))
            .attributes;
      const std::string s_shape = lookup_or(s_attrs, std::string_view(ir_attrs::k_shape), "ellipse");
      const std::string t_shape = lookup_or(t_attrs, std::string_view(ir_attrs::k_shape), "ellipse");

      double t_source = 0.0;
      double t_target = 0.0;
      if (s_shape == "circle") {
        const double r = std::max(state.node_w, state.node_h) / 2.0;
        t_source = compute_t_ellipse(nx, ny, r, r);
      } else if (s_shape == "ellipse") {
        t_source = compute_t_ellipse(nx, ny, half_x, half_y);
      } else {
        t_source = compute_t_rect(nx, ny, half_x, half_y);
      }
      if (t_shape == "circle") {
        const double r = std::max(state.node_w, state.node_h) / 2.0;
        t_target = compute_t_ellipse(-nx, -ny, r, r);
      } else if (t_shape == "ellipse") {
        t_target = compute_t_ellipse(-nx, -ny, half_x, half_y);
      } else {
        t_target = compute_t_rect(-nx, -ny, half_x, half_y);
      }
      const double x1 = sx + nx * t_source;
      const double y1 = sy + ny * t_source;
      const double x2 = tx - nx * t_target;
      const double y2 = ty - ny * t_target;

      const std::string style = lookup_or(amap, std::string_view(ir_attrs::k_style), "");
      std::string dash_attr;
      if (!style.empty()) {
        if (style.find("dotted") != std::string::npos)
          dash_attr = " stroke-dasharray=\"2,3\"";
        else if (style.find("dashed") != std::string::npos)
          dash_attr = " stroke-dasharray=\"6,4\"";
      }

      const std::string key = render_svg_detail::escape_xml(stroke) + std::string("|") +
                              render_svg_detail::escape_xml(penw);
      std::string marker_ref = "";
      auto mit = marker_id_for_style.find(key);
      if (mit != marker_id_for_style.end()) marker_ref = mit->second;

      os_inner << std::format(
          "  <line x1=\"{:.1f}\" y1=\"{:.1f}\" x2=\"{:.1f}\" y2=\"{:.1f}\" stroke=\"{}\" "
          "stroke-width=\"{}\" color=\"{}\"{} />\n",
          x1, y1, x2, y2, render_svg_detail::escape_xml(stroke), render_svg_detail::escape_xml(penw),
          render_svg_detail::escape_xml(stroke), dash_attr);

      if (!marker_ref.empty()) {
        const double marker_len = 8.0;
        const double sx_marker = x2 - nx * marker_len;
        const double sy_marker = y2 - ny * marker_len;
        const std::string seg = std::format(
            "  <line x1=\"{:.1f}\" y1=\"{:.1f}\" x2=\"{:.1f}\" y2=\"{:.1f}\" stroke=\"{}\" "
            "stroke-width=\"{}\" color=\"{}\"{} marker-end=\"url(#{})\" />\n",
            sx_marker, sy_marker, x2, y2, render_svg_detail::escape_xml(stroke),
            render_svg_detail::escape_xml(penw), render_svg_detail::escape_xml(stroke), dash_attr,
            marker_ref);
        marker_segments.push_back(seg);
      }

      if (amap.count(std::string(ir_attrs::k_label))) {
        const double lx = (x1 + x2) / 2.0;
        const double ly = (y1 + y2) / 2.0;
        os_inner << std::format(
            "  <text x=\"{:.1f}\" y=\"{:.1f}\" text-anchor=\"middle\" alignment-baseline=\"middle\" "
            "font-size=\"10\">{}</text>\n",
            lx, ly, render_svg_detail::escape_xml(amap.at(std::string(ir_attrs::k_label))));
      }
    }
    return marker_segments;
  };

  // Helper: render nodes group
  auto render_nodes = [&](std::ostream& os_inner,
                          const ir_graph& graph,
                          const RenderState& state,
                          const std::unordered_map<std::uint64_t, std::pair<double, double>>& ctrs) {
    for (const auto& n : graph.nodes) {
      const auto cid = n.id;
      const double cx = ctrs.at(cid).first;
      const double cy = ctrs.at(cid).second;
      const double x = cx - state.node_w / 2.0;
      const double y = cy - state.node_h / 2.0;

      const auto& amap = n.attributes;
      const std::string name = amap.count("name") ? amap.at("name") : std::format("n{}", n.id);
      const std::string id_safe = state.elem_id.at(cid);

      const std::string fill = lookup_or(amap, std::string_view(ir_attrs::k_fill_color), "#ffffff");
      const std::string stroke = lookup_or(amap, std::string_view(ir_attrs::k_color), "#000000");
      const std::string penw = lookup_or(amap, std::string_view(ir_attrs::k_pen_width), "1");
      const std::string fontsize = lookup_or(amap, std::string_view(ir_attrs::k_font_size), "12");
      const std::string fontname =
          lookup_or(amap, std::string_view(ir_attrs::k_font_name), "sans-serif");

      os_inner << std::format("  <g id=\"{}\">\n", id_safe);
      const std::string shape = lookup_or(amap, std::string_view(ir_attrs::k_shape), "ellipse");
      if (shape == "box") {
        os_inner << std::format(
            "    <rect x=\"{:.1f}\" y=\"{:.1f}\" width=\"{:.1f}\" height=\"{:.1f}\" rx=\"6\" "
            "ry=\"6\" fill=\"{}\" stroke=\"{}\" stroke-width=\"{}\" />\n",
            x, y, state.node_w, state.node_h, render_svg_detail::escape_xml(fill),
            render_svg_detail::escape_xml(stroke), render_svg_detail::escape_xml(penw));
      } else if (shape == "circle") {
        const double r = std::max(state.node_w, state.node_h) / 2.0;
        os_inner << std::format(
            "    <circle cx=\"{:.1f}\" cy=\"{:.1f}\" r=\"{:.1f}\" fill=\"{}\" stroke=\"{}\" "
            "stroke-width=\"{}\" />\n",
            cx, cy, r, render_svg_detail::escape_xml(fill), render_svg_detail::escape_xml(stroke),
            render_svg_detail::escape_xml(penw));
      } else if (shape == "ellipse") {
        os_inner << std::format(
            "    <ellipse cx=\"{:.1f}\" cy=\"{:.1f}\" rx=\"{:.1f}\" ry=\"{:.1f}\" fill=\"{}\" "
            "stroke=\"{}\" stroke-width=\"{}\" />\n",
            cx, cy, state.node_w / 2.0, state.node_h / 2.0, render_svg_detail::escape_xml(fill),
            render_svg_detail::escape_xml(stroke), render_svg_detail::escape_xml(penw));
      } else if (shape == "stadium") {
        const double rxv = state.node_h / 2.0;
        os_inner << std::format(
            "    <rect x=\"{:.1f}\" y=\"{:.1f}\" width=\"{:.1f}\" height=\"{:.1f}\" rx=\"{:.1f}\" "
            "ry=\"{:.1f}\" fill=\"{}\" stroke=\"{}\" stroke-width=\"{}\" />\n",
            x, y, state.node_w, state.node_h, rxv, rxv, render_svg_detail::escape_xml(fill),
            render_svg_detail::escape_xml(stroke), render_svg_detail::escape_xml(penw));
      } else if (shape == "diamond") {
        const double hx = state.node_w / 2.0;
        const double hy = state.node_h / 2.0;
        const double x1 = cx;
        const double y1 = cy - hy;
        const double x2 = cx + hx;
        const double y2 = cy;
        const double x3 = cx;
        const double y3 = cy + hy;
        const double x4 = cx - hx;
        const double y4 = cy;
        os_inner << std::format(
            "    <polygon points=\"{:.1f},{:.1f} {:.1f},{:.1f} {:.1f},{:.1f} {:.1f},{:.1f}\" "
            "fill=\"{}\" stroke=\"{}\" stroke-width=\"{}\" />\n",
            x1, y1, x2, y2, x3, y3, x4, y4, render_svg_detail::escape_xml(fill),
            render_svg_detail::escape_xml(stroke), render_svg_detail::escape_xml(penw));
      } else {
        os_inner << std::format(
            "    <rect x=\"{:.1f}\" y=\"{:.1f}\" width=\"{:.1f}\" height=\"{:.1f}\" rx=\"6\" "
            "ry=\"6\" fill=\"{}\" stroke=\"{}\" stroke-width=\"{}\" />\n",
            x, y, state.node_w, state.node_h, render_svg_detail::escape_xml(fill),
            render_svg_detail::escape_xml(stroke), render_svg_detail::escape_xml(penw));
      }

      const std::string label =
          amap.count(std::string(ir_attrs::k_label)) ? amap.at(std::string(ir_attrs::k_label)) : name;
      os_inner << std::format(
          "    <text x=\"{:.1f}\" y=\"{:.1f}\" text-anchor=\"middle\" alignment-baseline=\"middle\" "
          "font-family=\"{}\" font-size=\"{}\">{}</text>\n",
          cx, cy, render_svg_detail::escape_xml(fontname), render_svg_detail::escape_xml(fontsize),
          render_svg_detail::escape_xml(label));
      os_inner << "  </g>\n";
    }
  };

  // Build marker defs
  std::unordered_map<std::string, std::string> marker_id_for_style;
  std::vector<std::pair<std::string, std::string>> markers;
  build_markers(g, marker_id_for_style, markers);

  os << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  os << std::format(
      "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"{}\" height=\"{}\" viewBox=\"0 0 {} "
      "{}\">\n",
      static_cast<int>(std::ceil(st.svg_w)), static_cast<int>(std::ceil(st.svg_h)),
      static_cast<int>(std::ceil(st.svg_w)), static_cast<int>(std::ceil(st.svg_h)));

  if (!g.global_attrs.empty()) {
    auto it = g.global_attrs.find(std::string(ir_attrs::k_graph_label));
    if (it != g.global_attrs.end()) {
      os << "  <text x=\"" << (st.svg_w / 2.0) << "\" y=\"16\" text-anchor=\"middle\">"
         << render_svg_detail::escape_xml(it->second) << "</text>\n";
    }
  } else {
    os << "  <text x=\"" << (st.svg_w / 2.0) << "\" y=\"16\" text-anchor=\"middle\">"
       << render_svg_detail::escape_xml(title) << "</text>\n";
  }

  os << "  <rect width=\"100%\" height=\"100%\" fill=\"#ffffff\" />\n";
  os << "  <defs>\n";
  for (const auto& m : markers) {
    os << std::format(
        "    <marker id=\"{}\" viewBox=\"0 0 8 6\" markerWidth=\"8\" markerHeight=\"6\" refX=\"8\" "
        "refY=\"3\" orient=\"auto\" markerUnits=\"userSpaceOnUse\">\n",
        m.first);
    const auto col = render_svg_detail::escape_xml(m.second);
    os << std::format("      <path d=\"M0 0 L8 3 L0 6 z\" fill=\"{}\" stroke=\"{}\" />\n", col,
                      col);
    os << "    </marker>\n";
  }
  os << "  </defs>\n";

  // Render edges and collect marker segments
  auto marker_segments = render_edges(os, g, st, centers, marker_id_for_style);

  // Render nodes
  render_nodes(os, g, st, centers);

  for (const auto& seg : marker_segments) os << seg;
  os << "</svg>\n";
}

}  // namespace render_svg_detail

/**
 * @brief Render `ir_graph` to a simple SVG document.
 */
inline void render_svg(std::ostream& os, const ir_graph& g, std::string_view title = "DagIR") {
  // Create and populate the shared renderer state (constants, initial
  // positions, ranks and edge list).
  auto st = render_svg_detail::create_initial_state(g, title);

  // Resolve ordering, overlaps and compute preliminary centers.
  auto centers = render_svg_detail::resolve_overlaps_and_ordering(st, g);

  // Apply the Sugiyama layout which may override centers
  // and expand horizontal extents to avoid overlaps.
  render_svg_detail::apply_sugiyama_layout(g, st, centers);

  // Emit final SVG using the computed centers and state.
  render_svg_detail::emit_svg_output(os, g, st, centers, title);
}

}  // namespace dagir
