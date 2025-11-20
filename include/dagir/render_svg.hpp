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

}  // namespace render_svg_detail

/**
 * @brief Render `ir_graph` to a simple SVG document.
 */
inline void render_svg(std::ostream& os, const ir_graph& g, std::string_view title = "DagIR") {
  // Layout constants
  // Tuned to roughly match GraphViz `dot` default proportions used in
  // reference.svg: node radii ~30px and vertical radii ~18px.
  const double node_w = 70.0;  // width -> rx ~30
  const double node_h = 36.0;  // height -> ry ~18
  const double h_gap = 24.0;
  const double v_gap = 28.0 * 1.75;  // increase vertical gap by ~75%
  const double margin = 8.0;

  const size_t N = g.nodes.size();
  const size_t cols =
      std::max<size_t>(1, static_cast<size_t>(std::floor(std::sqrt(static_cast<double>(N)))));
  const size_t rows = (N + cols - 1) / cols;

  // Add extra padding equal to node dimensions so rounded corners and labels
  // are not clipped at the edges. Keep the original grid-based estimate
  // but expand by one node in each dimension.
  // Default estimated SVG size; may be expanded below to guarantee
  // minimum spacing between nodes when ranks are considered.
  double svg_w = margin * 2 + cols * node_w + (cols - 1) * h_gap + node_w;
  double svg_h =
      margin * 2 + rows * node_h + (rows - 1) * v_gap + 30.0 + node_h;  // space for title

  // (SVG header, title and defs are emitted after layout decisions below)

  // Build id lists and initial positions for force-directed layout
  std::vector<std::uint64_t> ids;
  ids.reserve(g.nodes.size());
  std::transform(g.nodes.begin(), g.nodes.end(), std::back_inserter(ids),
                 [](const auto& n) { return n.id; });

  // Map index -> node id and node attributes lookup
  std::unordered_map<std::uint64_t, const ir_attr_map*> attr_map;
  for (const auto& n : g.nodes) attr_map[n.id] = &n.attributes;

  // Compute drawing area (leave space for title at top). Reserve half a
  // node on each side so node rectangles do not get clipped.
  double area_w = svg_w - 2.0 * margin;
  double area_h = svg_h - 2.0 * margin - 24.0;
  const double half_node_w = node_w / 2.0;
  const double half_node_h = node_h / 2.0;
  double available_w = std::max(1.0, area_w - 2.0 * half_node_w);
  double available_h = std::max(1.0, area_h - 2.0 * half_node_h);
  double area = std::max(1.0, available_w * available_h);
  std::unordered_map<std::uint64_t, std::pair<double, double>> pos;
  std::mt19937_64 rng(1234567);
  std::uniform_real_distribution<double> ux(margin + half_node_w,
                                            margin + half_node_w + available_w);
  std::uniform_real_distribution<double> uy(margin + 24.0 + half_node_h,
                                            margin + 24.0 + half_node_h + available_h);
  // Determine ranks from attributes (fallback -1)
  std::unordered_map<std::uint64_t, int> rank_val;
  int min_rank = std::numeric_limits<int>::max();
  int max_rank = std::numeric_limits<int>::min();
  bool has_reachable = false;
  for (auto id : ids) {
    int r = -1;
    if (attr_map.count(id)) {
      const auto& amap = *attr_map[id];
      try {
        r = std::stoi(render_svg_detail::lookup_or(amap, std::string_view(ir_attrs::k_rank), "-1"));
      } catch (...) {
        r = -1;
      }
    }
    rank_val[id] = r;
    if (r >= 0) {
      has_reachable = true;
      min_rank = std::min(min_rank, r);
      max_rank = std::max(max_rank, r);
    }
  }

  // Build rank levels (place unreachable nodes in a final level)
  int levels = 1;
  if (has_reachable) {
    levels = max_rank - min_rank + 1;
  }
  const bool has_unreachable =
      std::any_of(ids.begin(), ids.end(), [&](std::uint64_t id) { return rank_val[id] < 0; });
  if (has_unreachable) ++levels;

  // Enforce minimum vertical spacing between ranks (node_h + 1/3 node_h)
  const double min_rank_v_step = node_h * (4.0 / 3.0);
  if (levels > 1) {
    const double required_v_span = min_rank_v_step * static_cast<double>(levels - 1);
    const double current_v_span = available_h;
    if (required_v_span > current_v_span) {
      const double extra_v = required_v_span - current_v_span;
      svg_h += extra_v;
      // recompute vertical available_h and area
      const double new_area_h = svg_h - 2.0 * margin - 24.0;
      const double new_available_h = std::max(1.0, new_area_h - 2.0 * half_node_h);
      // update uy distribution and helpers
      uy = std::uniform_real_distribution<double>(margin + 24.0 + half_node_h,
                                                  margin + 24.0 + half_node_h + new_available_h);
      // update local copies
      (void)new_area_h;
      available_h = new_available_h;
      area = std::max(1.0, available_w * available_h);
    }
  }

  // Compute Y coordinate for each rank index (0..levels-1). Anchor ranks
  // within the available height while leaving half-node padding top/bottom
  // so node rectangles are fully visible.
  std::unordered_map<int, double> rank_y;
  if (levels == 1) {
    rank_y[0] = margin + 24.0 + half_node_h + available_h / 2.0;
  } else {
    for (int i = 0; i < levels; ++i) {
      double tpos = static_cast<double>(i) / static_cast<double>(levels - 1);
      rank_y[i] = margin + 24.0 + half_node_h + tpos * available_h;
    }
  }

  // Initialize positions: x random, y determined by rank
  for (auto id : ids) {
    int r = rank_val[id];
    int idx = 0;
    if (r < 0) {
      idx = levels - 1;  // unreachable level at the end
    } else if (has_reachable) {
      idx = r - min_rank;
    }
    pos[id] = {ux(rng), rank_y[idx]};
  }

  // Ensure there is enough horizontal room to enforce minimum spacing
  // (node_w + 1/3 node_w between rectangles). If not, expand svg_w
  // and recompute available_w/ux distribution accordingly.
  const double min_center_step = node_w * (4.0 / 3.0);
  std::unordered_map<int, std::size_t> per_rank_count;
  for (auto id : ids) {
    int r = rank_val[id] < 0 ? levels - 1 : rank_val[id] - min_rank;
    per_rank_count[r] += 1;
  }
  const std::size_t max_per_rank =
      std::accumulate(per_rank_count.begin(), per_rank_count.end(), std::size_t{0},
                      [](std::size_t acc, const auto& kv) { return std::max(acc, kv.second); });
  if (max_per_rank > 1) {
    const double required_span = min_center_step * static_cast<double>(max_per_rank - 1);
    const double current_span = available_w;
    if (required_span > current_span) {
      // expand svg width to fit required span plus padding
      const double extra = required_span - current_span;
      svg_w += extra;
      // recompute area/available ranges based on new svg_w
      area_w = svg_w - 2.0 * margin;
      available_w = std::max(1.0, area_w - 2.0 * half_node_w);
      area = std::max(1.0, available_w * available_h);
      // adjust ux distribution to new available width and reinitialize X
      ux = std::uniform_real_distribution<double>(margin + half_node_w,
                                                  margin + half_node_w + available_w);
      for (auto id : ids) pos[id].first = ux(rng);
    }
  }

  // SVG header, title and defs will be emitted after layout so the final
  // `svg_w`/`svg_h` values are reflected in the output viewBox.

  // Build adjacency for edges
  std::vector<std::pair<std::uint64_t, std::uint64_t>> edges;
  edges.reserve(g.edges.size());
  std::transform(g.edges.begin(), g.edges.end(), std::back_inserter(edges),
                 [](const auto& e) { return std::make_pair(e.source, e.target); });

  // Force-directed layout (Fruchterman-Reingold)
  const double k = std::sqrt(area / std::max<size_t>(1, ids.size()));
  double t = std::max(area_w, area_h) / 10.0;
  const int iterations = std::clamp(100, 50, static_cast<int>(ids.size()) * 10);
  const double eps = 1e-6;

  for (int it = 0; it < iterations; ++it) {
    std::unordered_map<std::uint64_t, std::pair<double, double>> disp;
    for (auto id : ids) disp[id] = {0.0, 0.0};

    // repulsive forces
    for (size_t i = 0; i < ids.size(); ++i) {
      for (size_t j = i + 1; j < ids.size(); ++j) {
        auto u = ids[i];
        auto v = ids[j];
        double dx = pos[u].first - pos[v].first;
        double dy = pos[u].second - pos[v].second;
        double dist = std::sqrt(dx * dx + dy * dy) + eps;
        double force = (k * k) / dist;
        double nx = dx / dist;
        double ny = dy / dist;
        disp[u].first += nx * force;
        disp[u].second += ny * force;
        disp[v].first -= nx * force;
        disp[v].second -= ny * force;
      }
    }

    // attractive forces
    for (const auto& e : edges) {
      auto u = e.first;
      auto v = e.second;
      double dx = pos[u].first - pos[v].first;
      double dy = pos[u].second - pos[v].second;
      double dist = std::sqrt(dx * dx + dy * dy) + eps;
      double force = (dist * dist) / k;
      double nx = dx / dist;
      double ny = dy / dist;
      disp[u].first -= nx * force;
      disp[u].second -= ny * force;
      disp[v].first += nx * force;
      disp[v].second += ny * force;
    }

    // limit max displacement and apply
    for (auto id : ids) {
      double dx = disp[id].first;
      double dy = disp[id].second;
      double dist = std::sqrt(dx * dx + dy * dy) + eps;
      double limited = std::min(dist, t);
      // Apply only X displacement; Y is anchored to rank level
      pos[id].first += (dx / dist) * limited;
      // keep inside bounds (x only)
      pos[id].first =
          std::clamp(pos[id].first, margin + half_node_w, margin + half_node_w + available_w);
      // re-anchor Y to rank level
      int r = rank_val[id];
      int idx = 0;
      if (r < 0) {
        idx = levels - 1;
      } else if (has_reachable) {
        idx = r - min_rank;
      }
      pos[id].second = rank_y[idx];
    }

    // cool temperature
    t *= 0.95;
  }

  // centers now come from positions
  // Resolve overlaps within each rank by spacing node centers across the
  // available horizontal range for that rank. This replaces potentially
  // overlapping X coordinates with evenly-spaced centers while preserving
  // the per-rank grouping.
  std::unordered_map<int, std::vector<std::uint64_t>> by_rank;
  for (auto id : ids) {
    int r = rank_val[id];
    int idx = 0;
    if (r < 0) {
      idx = levels - 1;
    } else if (has_reachable) {
      idx = r - min_rank;
    }
    by_rank[idx].push_back(id);
  }

  // Build quick adjacency maps for barycenter heuristic
  std::unordered_map<std::uint64_t, std::vector<std::uint64_t>> outgoing, incoming;
  for (const auto& e : edges) {
    outgoing[e.first].push_back(e.second);
    incoming[e.second].push_back(e.first);
  }

  // Barycenter reordering: do a few top-down / bottom-up passes to reduce
  // inter-rank edge lengths and crossings. This only reorders nodes within
  // a rank (does not change ranks) using the average X position of
  // neighboring nodes in adjacent ranks as the sort key.
  // Use Sugiyama-style median heuristic with multiple passes for
  // crossing reduction. Median tends to produce layouts closer to
  // Graphviz `dot` engine's ordering than a simple average.
  const int reorder_passes = 8;
  auto rank_index_of = [&](std::uint64_t id) -> int {
    int r = rank_val[id];
    if (r < 0) return levels - 1;
    if (!has_reachable) return 0;
    return r - min_rank;
  };
  for (int pass = 0; pass < reorder_passes; ++pass) {
    // top-down: use neighbors in previous rank
    for (int r = 1; r < levels; ++r) {
      auto& vec = by_rank[r];
      if (vec.size() <= 1) continue;
      std::vector<std::pair<double, std::uint64_t>> order;
      order.reserve(vec.size());
      for (auto id : vec) {
        std::vector<double> xs;
        xs.reserve(incoming[id].size());
        for (auto nb : incoming[id]) {
          if (rank_index_of(nb) == r - 1) xs.push_back(pos[nb].first);
        }
        double key;
        if (xs.empty()) {
          key = pos[id].first;
        } else {
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

    // bottom-up: use neighbors in next rank
    for (int r = levels - 2; r >= 0; --r) {
      auto& vec = by_rank[r];
      if (vec.size() <= 1) continue;
      std::vector<std::pair<double, std::uint64_t>> order;
      order.reserve(vec.size());
      for (auto id : vec) {
        std::vector<double> xs;
        xs.reserve(outgoing[id].size());
        for (auto nb : outgoing[id]) {
          if (rank_index_of(nb) == r + 1) xs.push_back(pos[nb].first);
        }
        double key;
        if (xs.empty()) {
          key = pos[id].first;
        } else {
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

  // Transposition pass: try adjacent swaps inside each rank to reduce
  // number of edge crossings with neighboring ranks. Repeat until
  // stable or max iterations.
  auto count_crossings_between = [&](const std::vector<std::uint64_t>& left,
                                     const std::vector<std::uint64_t>& right) -> size_t {
    // build position map for right side
    std::unordered_map<std::uint64_t, size_t> pos_right;
    for (size_t i = 0; i < right.size(); ++i) pos_right[right[i]] = i;
    std::vector<size_t> targets;
    targets.reserve(left.size());
    for (auto u : left) {
      for (auto v : outgoing[u]) {
        auto it = pos_right.find(v);
        if (it != pos_right.end()) targets.push_back(it->second);
      }
    }
    // count inversions in targets using std::count_if
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
    // iterate ranks
    for (int r = 0; r < levels; ++r) {
      auto& vec = by_rank[r];
      if (vec.size() <= 1) continue;
      // consider crossings with previous and next rank
      std::vector<std::uint64_t> prev = (r > 0) ? by_rank[r - 1] : std::vector<std::uint64_t>();
      std::vector<std::uint64_t> next =
          (r + 1 < levels) ? by_rank[r + 1] : std::vector<std::uint64_t>();
      // try adjacent swaps
      for (size_t i = 0; i + 1 < vec.size(); ++i) {
        // current order
        size_t before = 0;
        if (!prev.empty()) before += count_crossings_between(prev, vec);
        if (!next.empty()) before += count_crossings_between(vec, next);
        // swap i and i+1
        std::swap(vec[i], vec[i + 1]);
        size_t after = 0;
        if (!prev.empty()) after += count_crossings_between(prev, vec);
        if (!next.empty()) after += count_crossings_between(vec, next);
        if (after < before) {
          swapped_any = true;
        } else {
          // revert
          std::swap(vec[i], vec[i + 1]);
        }
      }
    }
    if (!swapped_any) break;
  }

  const double left_center = margin + half_node_w;
  const double right_center = margin + half_node_w + available_w;
  for (auto& kv : by_rank) {
    auto& vec = kv.second;
    if (vec.empty()) continue;
    // sort by current X so relative ordering is stable
    std::sort(vec.begin(), vec.end(),
              [&](std::uint64_t a, std::uint64_t b) { return pos[a].first < pos[b].first; });
    const std::size_t m = vec.size();
    if (m == 1) {
      pos[vec[0]].first = std::clamp(pos[vec[0]].first, left_center, right_center);
    } else {
      const double span = right_center - left_center;
      const double min_step = node_w * (4.0 / 3.0);  // node width + 1/3 node space
      double step = span / static_cast<double>(m - 1);
      if (span >= min_step * static_cast<double>(m - 1)) {
        // we have enough room to enforce minimum spacing
        step = min_step;
        // center the block within [left_center, right_center]
        double block_width = step * static_cast<double>(m - 1);
        double start = left_center + (span - block_width) / 2.0;
        for (std::size_t i = 0; i < m; ++i)
          pos[vec[i]].first = start + step * static_cast<double>(i);
      } else {
        // fall back to evenly distributing across the available span
        for (std::size_t i = 0; i < m; ++i)
          pos[vec[i]].first = left_center + step * static_cast<double>(i);
      }
    }
  }

  std::unordered_map<std::uint64_t, std::pair<double, double>> centers = pos;
  // Allow runtime toggle between classic renderer layout and the new
  // Sugiyama layout. Set environment variable `DAGIR_SVG_LAYOUT=classic`
  // to keep the previous behavior.
  const char* dagir_svg_layout_env = std::getenv("DAGIR_SVG_LAYOUT");
  bool use_sugiyama = true;
  if (dagir_svg_layout_env && std::string(dagir_svg_layout_env) == "classic") use_sugiyama = false;

  // Use minimal in-repo Sugiyama layout for node placement when enabled.
  if (use_sugiyama) {
    dagir::sugiyama_options opts;
    opts.node_dist = h_gap;  // reuse renderer constants
    opts.layer_dist = v_gap;
    auto coords = dagir::sugiyama_layout_compute(g, opts);
    // coords.x/y are indexed by node index (g.nodes order).
    // Remap sugiyama coordinates into the renderer's available drawing area.
    // This also expands `svg_w` when necessary to ensure a minimum horizontal
    // spacing between node centers in the same rank so nodes don't overlap.
    double sug_left_center = margin + half_node_w;
    double sug_right_center = margin + half_node_w + available_w;

    // Compute per-rank counts to decide required horizontal span
    std::unordered_map<int, std::size_t> per_rank_count_sug;
    auto rank_index_of_index = [&](size_t idx) -> int {
      int r = -1;
      if (idx < g.nodes.size()) {
        const auto& amap = g.nodes[idx].attributes;
        try {
          r = std::stoi(
              render_svg_detail::lookup_or(amap, std::string_view(ir_attrs::k_rank), "-1"));
        } catch (...) {
          r = -1;
        }
      }
      if (r < 0) return levels - 1;
      if (!has_reachable) return 0;
      return r - min_rank;
    };
    for (size_t i = 0; i < coords.x.size() && i < g.nodes.size(); ++i) {
      int ridx = rank_index_of_index(i);
      per_rank_count_sug[ridx] += 1;
    }
    const std::size_t max_per_rank_sug =
        std::accumulate(per_rank_count_sug.begin(), per_rank_count_sug.end(), std::size_t{0},
                        [](std::size_t acc, const auto& kv) { return std::max(acc, kv.second); });

    if (max_per_rank_sug > 1) {
      const double required_span = min_center_step * static_cast<double>(max_per_rank_sug - 1);
      const double current_span = sug_right_center - sug_left_center;
      if (required_span > current_span) {
        const double extra = required_span - current_span;
        svg_w += extra;
        // recompute drawing extents
        area_w = svg_w - 2.0 * margin;
        available_w = std::max(1.0, area_w - 2.0 * half_node_w);
        sug_right_center = margin + half_node_w + available_w;
      }
    }

    // Per-rank placement: respect Sugiyama ordering but place nodes in
    // centered blocks per rank to enforce minimum spacing without
    // expanding too much horizontally. Before final placement perform a
    // top-down pass that orders nodes in each rank by the average X of
    // their parents so children are centered under their parents.
    // Anchor Y coordinates to the renderer's `rank_y` to increase
    // vertical separation between ranks.
    std::unordered_map<int, std::vector<std::pair<double, size_t>>> sug_by_rank;
    for (size_t i = 0; i < coords.x.size() && i < g.nodes.size(); ++i) {
      int ridx = rank_index_of_index(i);
      sug_by_rank[ridx].emplace_back(coords.x[i], i);
    }

    // Compute adjacency map from node index to parent indices for averaging
    std::unordered_map<size_t, std::vector<size_t>> parents_of_index;
    std::unordered_map<std::uint64_t, size_t> id_to_index;
    for (size_t i = 0; i < g.nodes.size(); ++i) id_to_index[g.nodes[i].id] = i;
    for (const auto& e : g.edges) {
      auto it_s = id_to_index.find(e.source);
      auto it_t = id_to_index.find(e.target);
      if (it_s != id_to_index.end() && it_t != id_to_index.end()) {
        parents_of_index[it_t->second].push_back(it_s->second);
      }
    }

    // (Removed top-down child-centering pass)

    const double min_step = node_w * (4.0 / 3.0);
    for (auto& kv : sug_by_rank) {
      int ridx = kv.first;
      auto& vec = kv.second;
      if (vec.empty()) continue;
      // sort by sugiyama X to preserve relative ordering
      std::sort(vec.begin(), vec.end(),
                [](const auto& a, const auto& b) { return a.first < b.first; });
      const size_t m = vec.size();
      double span = sug_right_center - sug_left_center;
      double step = (m <= 1) ? 0.0 : (span / static_cast<double>(m - 1));
      bool enforce_min = (span >= min_step * static_cast<double>(m - 1));
      double start;
      if (m == 1) {
        start = sug_left_center + span * 0.5;
        centers[g.nodes[vec[0].second].id] = {start, rank_y[ridx]};
      } else {
        if (enforce_min) {
          step = min_step;
          double block_width = step * static_cast<double>(m - 1);
          start = sug_left_center + (span - block_width) / 2.0;
        } else {
          // distribute evenly across span
          start = sug_left_center;
        }
        for (size_t i = 0; i < m; ++i) {
          double nx = start + step * static_cast<double>(i);
          centers[g.nodes[vec[i].second].id] = {nx, rank_y[ridx]};
        }
      }
    }
  }
  std::unordered_map<std::uint64_t, std::string> elem_id;
  for (const auto& n : g.nodes) elem_id[n.id] = std::format("dagir-{}", n.id);

  // Now that layout is finalized (centers computed and svg_w/svg_h possibly
  // adjusted), emit the SVG header, title and defs so the viewBox matches.
  os << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
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

  // Render edges first so nodes appear in front
  for (const auto& e : g.edges) {
    const auto s_it = centers.find(e.source);
    const auto t_it = centers.find(e.target);
    if (s_it == centers.end() || t_it == centers.end()) continue;
    double sx = s_it->second.first;
    double sy = s_it->second.second;
    double tx = t_it->second.first;
    double ty = t_it->second.second;

    const auto& amap = e.attributes;
    const std::string stroke =
        render_svg_detail::lookup_or(amap, std::string_view(ir_attrs::k_color), "#000000");
    const std::string penw =
        render_svg_detail::lookup_or(amap, std::string_view(ir_attrs::k_pen_width), "1");

    // Compute shortened segment endpoints so lines stop at node rectangle
    // boundaries rather than centers. Use rectangle half-extents.
    const double dx = tx - sx;
    const double dy = ty - sy;
    double len = std::sqrt(dx * dx + dy * dy);
    if (len < 1e-6) continue;
    const double nx = dx / len;
    const double ny = dy / len;
    const double half_x = node_w / 2.0;
    const double half_y = node_h / 2.0;
    auto compute_t_rect = [&](double nx, double ny, double hx, double hy) {
      const double denom_nx = std::abs(nx) < 1e-9 ? 1e9 : std::abs(nx);
      const double denom_ny = std::abs(ny) < 1e-9 ? 1e9 : std::abs(ny);
      return std::min(hx / denom_nx, hy / denom_ny);
    };
    auto compute_t_ellipse = [&](double nx, double ny, double rx, double ry) {
      // Solve (rx*nx*t)^2 + (ry*ny*t)^2 = 1  => t = 1 / sqrt((rx*nx)^2 + (ry*ny)^2)
      const double a = (rx * nx);
      const double b = (ry * ny);
      const double denom = std::sqrt(a * a + b * b);
      return (denom < 1e-12) ? 0.0 : (1.0 / denom);
    };

    // determine shapes for source and target
    const auto& s_attrs =
        g.nodes
            .at(std::distance(g.nodes.begin(),
                              std::find_if(g.nodes.begin(), g.nodes.end(),
                                           [&](const auto& n) { return n.id == e.source; })))
            .attributes;
    const auto& t_attrs =
        g.nodes
            .at(std::distance(g.nodes.begin(),
                              std::find_if(g.nodes.begin(), g.nodes.end(),
                                           [&](const auto& n) { return n.id == e.target; })))
            .attributes;
    const std::string s_shape =
        render_svg_detail::lookup_or(s_attrs, std::string_view(ir_attrs::k_shape), "ellipse");
    const std::string t_shape =
        render_svg_detail::lookup_or(t_attrs, std::string_view(ir_attrs::k_shape), "ellipse");

    double t_source = 0.0;
    double t_target = 0.0;
    if (s_shape == "circle") {
      const double r = std::max(node_w, node_h) / 2.0;
      t_source = compute_t_ellipse(nx, ny, r, r);
    } else if (s_shape == "ellipse") {
      t_source = compute_t_ellipse(nx, ny, half_x, half_y);
    } else {
      // rectangle-like shapes
      t_source = compute_t_rect(nx, ny, half_x, half_y);
    }
    if (t_shape == "circle") {
      const double r = std::max(node_w, node_h) / 2.0;
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

    // Edge style: map k_style tokens to stroke-dasharray for SVG
    const std::string style =
        render_svg_detail::lookup_or(amap, std::string_view(ir_attrs::k_style), "");
    std::string dash_attr;
    if (!style.empty()) {
      if (style.find("dotted") != std::string::npos) {
        dash_attr = " stroke-dasharray=\"2,3\"";
      } else if (style.find("dashed") != std::string::npos) {
        dash_attr = " stroke-dasharray=\"6,4\"";
      }
    }

    os << std::format(
        "  <line x1=\"{:.1f}\" y1=\"{:.1f}\" x2=\"{:.1f}\" y2=\"{:.1f}\" stroke=\"{}\" "
        "stroke-width=\"{}\"{} marker-end=\"url(#dagir-arrow)\" />\n",
        x1, y1, x2, y2, render_svg_detail::escape_xml(stroke), render_svg_detail::escape_xml(penw),
        dash_attr);

    if (amap.count(std::string(ir_attrs::k_label))) {
      const double lx = (x1 + x2) / 2.0;
      const double ly = (y1 + y2) / 2.0 - 6.0;
      os << std::format(
          "  <text x=\"{:.1f}\" y=\"{:.1f}\" text-anchor=\"middle\" font-size=\"10\">{}</text>\n",
          lx, ly, render_svg_detail::escape_xml(amap.at(std::string(ir_attrs::k_label))));
    }
  }

  // Render nodes on top of edges
  for (const auto& n : g.nodes) {
    const auto cid = n.id;
    const double cx = centers[cid].first;
    const double cy = centers[cid].second;
    const double x = cx - node_w / 2.0;
    const double y = cy - node_h / 2.0;

    const auto& amap = n.attributes;
    const std::string name = amap.count("name") ? amap.at("name") : std::format("n{}", n.id);
    const std::string id_safe = elem_id[cid];

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

    os << std::format("  <g id=\"{}\">\n", id_safe);
    const std::string shape =
        render_svg_detail::lookup_or(amap, std::string_view(ir_attrs::k_shape), "ellipse");
    if (shape == "box") {
      os << std::format(
          "    <rect x=\"{:.1f}\" y=\"{:.1f}\" width=\"{:.1f}\" height=\"{:.1f}\" rx=\"6\" "
          "ry=\"6\" "
          "fill=\"{}\" stroke=\"{}\" stroke-width=\"{}\" />\n",
          x, y, node_w, node_h, render_svg_detail::escape_xml(fill),
          render_svg_detail::escape_xml(stroke), render_svg_detail::escape_xml(penw));
    } else if (shape == "circle") {
      const double r = std::max(node_w, node_h) / 2.0;
      os << std::format(
          "    <circle cx=\"{:.1f}\" cy=\"{:.1f}\" r=\"{:.1f}\" fill=\"{}\" stroke=\"{}\" "
          "stroke-width=\"{}\" />\n",
          cx, cy, r, render_svg_detail::escape_xml(fill), render_svg_detail::escape_xml(stroke),
          render_svg_detail::escape_xml(penw));
    } else if (shape == "ellipse") {
      os << std::format(
          "    <ellipse cx=\"{:.1f}\" cy=\"{:.1f}\" rx=\"{:.1f}\" ry=\"{:.1f}\" fill=\"{}\" "
          "stroke=\"{}\" stroke-width=\"{}\" />\n",
          cx, cy, node_w / 2.0, node_h / 2.0, render_svg_detail::escape_xml(fill),
          render_svg_detail::escape_xml(stroke), render_svg_detail::escape_xml(penw));
    } else if (shape == "stadium") {
      // stadium: rounded rect with large rx so ends are semicircles
      const double rxv = node_h / 2.0;
      os << std::format(
          "    <rect x=\"{:.1f}\" y=\"{:.1f}\" width=\"{:.1f}\" height=\"{:.1f}\" rx=\"{:.1f}\" "
          "ry=\"{:.1f}\" "
          "fill=\"{}\" stroke=\"{}\" stroke-width=\"{}\" />\n",
          x, y, node_w, node_h, rxv, rxv, render_svg_detail::escape_xml(fill),
          render_svg_detail::escape_xml(stroke), render_svg_detail::escape_xml(penw));
    } else if (shape == "diamond") {
      // diamond centered at (cx,cy)
      const double hx = node_w / 2.0;
      const double hy = node_h / 2.0;
      const double x1 = cx;
      const double y1 = cy - hy;
      const double x2 = cx + hx;
      const double y2 = cy;
      const double x3 = cx;
      const double y3 = cy + hy;
      const double x4 = cx - hx;
      const double y4 = cy;
      os << std::format(
          "    <polygon points=\"{:.1f},{:.1f} {:.1f},{:.1f} {:.1f},{:.1f} {:.1f},{:.1f}\" "
          "fill=\"{}\" stroke=\"{}\" stroke-width=\"{}\" />\n",
          x1, y1, x2, y2, x3, y3, x4, y4, render_svg_detail::escape_xml(fill),
          render_svg_detail::escape_xml(stroke), render_svg_detail::escape_xml(penw));
    } else {
      // fallback to rounded rectangle
      os << std::format(
          "    <rect x=\"{:.1f}\" y=\"{:.1f}\" width=\"{:.1f}\" height=\"{:.1f}\" rx=\"6\" "
          "ry=\"6\" "
          "fill=\"{}\" stroke=\"{}\" stroke-width=\"{}\" />\n",
          x, y, node_w, node_h, render_svg_detail::escape_xml(fill),
          render_svg_detail::escape_xml(stroke), render_svg_detail::escape_xml(penw));
    }

    const std::string label =
        amap.count(std::string(ir_attrs::k_label)) ? amap.at(std::string(ir_attrs::k_label)) : name;
    os << std::format(
        "    <text x=\"{:.1f}\" y=\"{:.1f}\" text-anchor=\"middle\" alignment-baseline=\"middle\" "
        "font-family=\"{}\" font-size=\"{}\">{}</text>\n",
        cx, cy, render_svg_detail::escape_xml(fontname), render_svg_detail::escape_xml(fontsize),
        render_svg_detail::escape_xml(label));
    os << "  </g>\n";
  }

  os << "</svg>\n";
}

}  // namespace dagir
