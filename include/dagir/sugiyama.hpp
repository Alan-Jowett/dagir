/**
 * @file
 * @brief
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include <algorithm>
#include <dagir/ir.hpp>
#include <numeric>
#include <optional>
#include <vector>

namespace dagir {

struct sugiyama_options {
  bool use_dummy_nodes = true;
  unsigned transpose_iters = 10;
  double node_dist = 24.0;   // default horizontal gap
  double layer_dist = 24.0;  // default vertical gap
};

// hierarchy produced from graph and optional rank attribute
struct hierarchy {
  std::vector<std::vector<std::size_t>> layers;  // store indices into g.nodes
  std::vector<int> rank_of;                      // index by node index
};

// Build layers from graph using node attribute `k_rank` if present, otherwise fallback to BFS
// layering
hierarchy build_hierarchy(const ir_graph& g) {
  hierarchy h;
  int n = (int)g.nodes.size();
  h.rank_of.assign(n, std::numeric_limits<int>::min());

  // build id->index map
  std::unordered_map<std::uint64_t, int> id2idx;
  id2idx.reserve(n);
  for (int i = 0; i < n; ++i) id2idx[g.nodes[i].id] = i;

  // build adjacency lists
  std::vector<std::vector<int>> out_adj(n), in_adj(n);
  for (const auto& e : g.edges) {
    auto si = id2idx.at(e.source);
    auto ti = id2idx.at(e.target);
    out_adj[si].push_back(ti);
    in_adj[ti].push_back(si);
  }

  // try to use attribute k_rank
  bool has_rank = false;
  for (int u = 0; u < n; ++u) {
    if (g.nodes[u].attributes.find(std::string(ir_attrs::k_rank)) != g.nodes[u].attributes.end()) {
      has_rank = true;
      break;
    }
  }
  if (has_rank) {
    int minr = std::numeric_limits<int>::max();
    for (int u = 0; u < n; ++u) {
      auto it = g.nodes[u].attributes.find(std::string(ir_attrs::k_rank));
      if (it != g.nodes[u].attributes.end()) {
        int r = std::stoi(it->second);
        h.rank_of[u] = r;
        minr = std::min(minr, r);
      }
    }
    // normalize
    for (int u = 0; u < n; ++u) {
      if (h.rank_of[u] > std::numeric_limits<int>::min()) h.rank_of[u] -= minr;
    }
    int maxr = 0;
    for (int u = 0; u < n; ++u)
      if (h.rank_of[u] > std::numeric_limits<int>::min()) maxr = std::max(maxr, h.rank_of[u]);
    h.layers.resize(maxr + 1);
    for (int u = 0; u < n; ++u)
      if (h.rank_of[u] > std::numeric_limits<int>::min()) h.layers[h.rank_of[u]].push_back(u);
    // put unranked nodes into layer 0
    for (int u = 0; u < n; ++u)
      if (h.rank_of[u] == std::numeric_limits<int>::min()) {
        h.rank_of[u] = 0;
        h.layers[0].push_back(u);
      }
  } else {
    // simple BFS layering: sources at layer 0, then increasing
    std::vector<int> indeg(n, 0);
    for (int u = 0; u < n; ++u)
      for (auto v : out_adj[u]) ++indeg[v];
    std::vector<int> q;
    for (int u = 0; u < n; ++u)
      if (indeg[u] == 0) {
        q.push_back(u);
        h.rank_of[u] = 0;
      }
    int processed = 0;
    int curr_rank = 0;
    while (!q.empty()) {
      std::vector<int> next;
      for (int u : q) {
        h.layers.resize(std::max((size_t)h.layers.size(), (size_t)(curr_rank + 1)));
        h.layers[curr_rank].push_back(u);
        for (auto v : out_adj[u]) {
          --indeg[v];
          if (indeg[v] == 0) {
            next.push_back(v);
            h.rank_of[v] = curr_rank + 1;
          }
        }
        ++processed;
      }
      ++curr_rank;
      q.swap(next);
    }
    // nodes not reached (cycles) into last layer
    if (processed < n) {
      for (int u = 0; u < n; ++u)
        if (h.rank_of[u] == std::numeric_limits<int>::min()) {
          h.layers.resize(h.layers.size() + 1);
          h.rank_of[u] = (int)h.layers.size() - 1;
          h.layers.back().push_back(u);
        }
    }
  }
  return h;
}

// split long edges by inserting dummy ids (we can't mutate original graph; instead return long-edge
// paths) For minimal implementation we do not actually create dummy nodes in graph, but we record
// long edges to expand when routing.
using long_edge = std::pair<ir_edge, std::vector<std::size_t>>;
std::vector<long_edge> find_long_edges(const ir_graph& g, const hierarchy& h) {
  std::vector<long_edge> out;
  int n = (int)g.nodes.size();
  std::unordered_map<std::uint64_t, int> id2idx;
  id2idx.reserve(n);
  for (int i = 0; i < n; ++i) id2idx[g.nodes[i].id] = i;
  for (const auto& e : g.edges) {
    int u = id2idx.at(e.source);
    int v = id2idx.at(e.target);
    int span = h.rank_of[v] - h.rank_of[u];
    if (span > 1) out.push_back({{e.source, e.target}, {}});
  }
  return out;
}

// Count crossings between two nodes u and v on same layer given positions mapping
int crossing_number(const hierarchy& h, const ir_graph& g, int layer_idx, std::size_t pos_u,
                    std::size_t pos_v, const std::vector<int>& pos_in_layer) {
  // pos_in_layer maps node -> index in its layer
  int count = 0;
  auto const& layer = h.layers[layer_idx];
  std::size_t u = layer[pos_u];
  std::size_t v = layer[pos_v];
  // build id->index map
  int n = (int)g.nodes.size();
  std::unordered_map<std::uint64_t, int> id2idx;
  id2idx.reserve(n);
  for (int i = 0; i < n; ++i) id2idx[g.nodes[i].id] = i;
  // build adjacency quick
  std::vector<std::vector<int>> out_adj(n), in_adj(n);
  for (const auto& e : g.edges) {
    int si = id2idx[e.source];
    int ti = id2idx[e.target];
    out_adj[si].push_back(ti);
    in_adj[ti].push_back(si);
  }
  for (auto out_u : out_adj[u]) {
    count += std::count_if(out_adj[v].begin(), out_adj[v].end(),
                           [&](int out_v) { return pos_in_layer[out_v] < pos_in_layer[out_u]; });
  }
  for (auto in_u : in_adj[u]) {
    count += std::count_if(in_adj[v].begin(), in_adj[v].end(),
                           [&](int in_v) { return pos_in_layer[in_v] < pos_in_layer[in_u]; });
  }
  return count;
}

// Barycentric reordering + transpose
void barycentric_reorder(hierarchy& h, const ir_graph& g, unsigned transpose_iters = 5) {
  int L = (int)h.layers.size();
  int n = (int)g.nodes.size();
  std::vector<int> pos_in_layer(n, -1);
  std::unordered_map<std::uint64_t, int> id2idx;
  id2idx.reserve(n);
  for (int i = 0; i < n; ++i) id2idx[g.nodes[i].id] = i;
  std::vector<std::vector<int>> out_adj(n), in_adj(n);
  for (const auto& e : g.edges) {
    int si = id2idx[e.source];
    int ti = id2idx[e.target];
    out_adj[si].push_back(ti);
    in_adj[ti].push_back(si);
  }

  auto update_pos = [&]() {
    for (int li = 0; li < L; ++li)
      for (int i = 0; i < (int)h.layers[li].size(); ++i) pos_in_layer[h.layers[li][i]] = i;
  };

  update_pos();

  // iterative passes: top-down then bottom-up
  for (int pass = 0; pass < 2; ++pass) {
    if (pass == 0) {
      for (int li = 1; li < L; ++li) {
        // compute barycenters from incoming neighbors
        std::vector<std::pair<double, std::size_t>> arr;
        arr.reserve(h.layers[li].size());
        for (auto u : h.layers[li]) {
          double sum = 0;
          int cnt = 0;
          for (auto p : in_adj[u]) {
            if (pos_in_layer[p] >= 0) {
              sum += pos_in_layer[p];
              ++cnt;
            }
          }
          double w =
              (cnt == 0) ? static_cast<double>(pos_in_layer[u]) : sum / static_cast<double>(cnt);
          arr.push_back({w, u});
        }
        std::stable_sort(arr.begin(), arr.end(),
                         [](const auto& a, const auto& b) { return a.first < b.first; });
        for (int i = 0; i < (int)arr.size(); ++i) h.layers[li][i] = arr[i].second;
        update_pos();
      }
    } else {
      for (int li = L - 2; li >= 0; --li) {
        std::vector<std::pair<double, std::size_t>> arr;
        arr.reserve(h.layers[li].size());
        for (auto u : h.layers[li]) {
          double sum = 0;
          int cnt = 0;
          for (auto p : out_adj[u]) {
            if (pos_in_layer[p] >= 0) {
              sum += pos_in_layer[p];
              ++cnt;
            }
          }
          double w =
              (cnt == 0) ? static_cast<double>(pos_in_layer[u]) : sum / static_cast<double>(cnt);
          arr.push_back({w, u});
        }
        std::stable_sort(arr.begin(), arr.end(),
                         [](const auto& a, const auto& b) { return a.first < b.first; });
        for (int i = 0; i < (int)arr.size(); ++i) h.layers[li][i] = arr[i].second;
        update_pos();
      }
    }
  }

  // transpose passes to locally reduce crossings
  for (unsigned it = 0; it < transpose_iters; ++it) {
    bool improved = false;
    for (int li = 0; li < L; ++li) {
      update_pos();
      auto& layer = h.layers[li];
      for (int i = 0; i + 1 < (int)layer.size(); ++i) {
        int oldc = crossing_number(h, g, li, static_cast<std::size_t>(i),
                                   static_cast<std::size_t>(i + 1), pos_in_layer);
        // swap and test
        std::swap(layer[i], layer[i + 1]);
        update_pos();
        int newc = crossing_number(h, g, li, static_cast<std::size_t>(i),
                                   static_cast<std::size_t>(i + 1), pos_in_layer);
        if (newc < oldc) {
          improved = true;
        } else {
          std::swap(layer[i], layer[i + 1]);
          update_pos();
        }
      }
    }
    if (!improved) break;
  }
}

// Simple positioning: place nodes per layer left-to-right with equal spacing; center nodes by layer
// Simple positioning: place nodes per layer left-to-right with equal spacing; center nodes by layer
struct coords {
  std::vector<double> x;  // by node id
  std::vector<double> y;  // by node id
};

coords simple_positioning(const hierarchy& h, const ir_graph& g, const sugiyama_options& opt) {
  int n = (int)g.nodes.size();
  coords c;
  c.x.assign(n, 0.0);
  c.y.assign(n, 0.0);
  double y = 0.0;
  for (int li = 0; li < (int)h.layers.size(); ++li) {
    auto const& layer = h.layers[li];
    double x = 0.0;
    for (int i = 0; i < (int)layer.size(); ++i) {
      std::size_t u = layer[i];
      c.x[u] = x;
      c.y[u] = y;
      x += opt.node_dist;
    }
    // center layer by subtracting half-width
    if (!layer.empty()) {
      double minx = c.x[layer.front()];
      double maxx = c.x[layer.back()];
      double mid = (minx + maxx) / 2.0;
      double shift = -mid;
      for (auto u : layer) c.x[u] += shift;
    }
    y += opt.layer_dist;
  }
  return c;
}

// Top-level API: compute sugiyama coords from graph
coords sugiyama_layout_compute(const ir_graph& g, const sugiyama_options& opt = {}) {
  auto h = build_hierarchy(g);
  auto long_edges = find_long_edges(g, h);
  (void)long_edges;
  barycentric_reorder(h, g, opt.transpose_iters);
  auto c = simple_positioning(h, g, opt);
  // note: we don't handle routing or dummy node expansion here; renderer can use coords for node
  // centers
  return c;
}

}  // namespace dagir
