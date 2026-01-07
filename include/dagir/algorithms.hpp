/**
 * @file algorithms.hpp
 * @brief Graph algorithms operating on `read_only_dag_view` adapters.
 *
 * Contains helpers like `kahn_topological_order` and `postorder_fold` which
 * operate on types modeling `dagir::concepts::read_only_dag_view`.
 *
 * SPDX-License-Identifier: MIT
 * © DagIR Contributors. All rights reserved.
 */

#pragma once

#include <functional>
#include <queue>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "dagir/concepts/read_only_dag_view.hpp"

namespace dagir {

// Forward declarations for hash helper
namespace detail {
  struct pair_hash {
    template <class T1, class T2>
    std::size_t operator()(const std::pair<T1, T2>& p) const {
      auto h1 = std::hash<T1>{}(p.first);
      auto h2 = std::hash<T2>{}(p.second);
      return h1 ^ (h2 << 1);
    }
  };
}

/**
 * @brief Result of cycle detection analysis.
 *
 * Contains information about cycles detected in the graph including
 * back-edges and strongly connected components.
 */
struct cycle_info {
  /** @brief Whether the graph contains any cycles */
  bool has_cycles = false;
  
  /** @brief Map from edge (source_key, target_key) to whether it's a back-edge */
  std::unordered_map<std::pair<std::uint64_t, std::uint64_t>, bool, 
                     detail::pair_hash> back_edges;
  
  /** @brief Map from node key to its cycle group (SCC identifier) */
  std::unordered_map<std::uint64_t, std::size_t> cycle_groups;
  
  /** @brief Number of strongly connected components */
  std::size_t num_sccs = 0;
};

/**
 * @brief Compute a topological ordering of nodes reachable from `view.roots()`
 *        using Kahn's algorithm.
 *
 * @tparam View A type modeling ::dagir::read_only_dag_view
 * @param view The read-only DAG view
 * @return std::vector<typename View::handle> A topological ordering of handles.
 * @throws std::runtime_error if a cycle is detected in the reachable subgraph.
 *
 * Notes:
 *  - This function traverses the reachable subgraph starting from `view.roots()`.
 *  - Nodes are identified by their `stable_key()` for hash maps, and the returned
 *    handles preserve the adapter's handle values.
 */
template <dagir::concepts::read_only_dag_view View>
std::vector<typename View::handle> kahn_topological_order(const View& view) {
  using H = typename View::handle;
  using key_t = std::uint64_t;

  std::unordered_map<key_t, std::size_t> indeg;
  std::unordered_map<key_t, H> handle_of;
  std::unordered_set<key_t> seen;
  std::vector<key_t> nodes;  // all discovered node keys (for iteration)

  // helper to extract a child handle from a range element (edge or handle).
  auto extract_child = []<class E>(const E& e) -> H {
    if constexpr (std::convertible_to<E, H>) {
      return static_cast<H>(e);
    } else {
      return e.target();
    }
  };

  // BFS/stack from roots to discover reachable nodes and compute indegrees
  std::vector<H> work;
  for (auto const& r : view.roots()) {
    H h = r;
    key_t k = h.stable_key();
    if (seen.insert(k).second) {
      handle_of.emplace(k, h);
      indeg.try_emplace(k, 0);
      nodes.push_back(k);
      work.push_back(h);
    }
  }

  for (std::size_t i = 0; i < work.size(); ++i) {
    H cur = work[i];
    for (auto const& edge_like : view.children(cur)) {
      H child = extract_child(edge_like);
      key_t ck = child.stable_key();
      // ensure child is in maps
      handle_of.try_emplace(ck, child);
      auto [it, inserted] = indeg.try_emplace(ck, 0);
      ++(it->second);

      if (seen.insert(ck).second) {
        nodes.push_back(ck);
        work.push_back(child);
      }
    }
  }

  // Kahn: push all zero-indegree nodes
  std::queue<key_t> q;
  for (auto const& k : nodes) {
    if (indeg[k] == 0) q.push(k);
  }

  std::vector<H> order;
  order.reserve(nodes.size());

  while (!q.empty()) {
    key_t k = q.front();
    q.pop();
    order.push_back(handle_of.at(k));

    H h = handle_of.at(k);
    for (auto const& edge_like : view.children(h)) {
      H child = extract_child(edge_like);
      key_t ck = child.stable_key();
      auto it = indeg.find(ck);
      if (it == indeg.end()) continue;  // child outside discovered set
      if (--(it->second) == 0) q.push(ck);
    }
  }

  if (order.size() != nodes.size())
    throw std::runtime_error("kahn_topological_order: cycle detected in reachable graph");

  return order;
}

/**
 * @brief Detect cycles in the graph using depth-first search.
 *
 * @tparam View A type modeling ::dagir::read_only_dag_view
 * @param view The read-only DAG/DCG view
 * @return cycle_info Structure containing cycle detection results
 *
 * Uses DFS with visited/recursion stack tracking to identify:
 * - Back-edges (edges that create cycles)
 * - Strongly connected components using Tarjan's algorithm
 * - Which nodes belong to each SCC
 *
 * Notes:
 *  - This function works for both DAGs and DCGs
 *  - For DAGs, returns has_cycles=false with empty back_edges
 *  - For DCGs, identifies all cycles and their components
 */
template <dagir::concepts::read_only_dag_view View>
cycle_info detect_cycles(const View& view) {
  using H = typename View::handle;
  using key_t = std::uint64_t;
  
  cycle_info result;
  
  // DFS state tracking
  enum class State { Unvisited, Visiting, Visited };
  std::unordered_map<key_t, State> state;
  std::unordered_map<key_t, H> handle_of;
  
  // Tarjan's algorithm state
  std::unordered_map<key_t, std::size_t> dfs_num;
  std::unordered_map<key_t, std::size_t> low_link;
  std::vector<key_t> stack;
  std::unordered_set<key_t> on_stack;
  std::size_t counter = 0;
  
  // Helper to extract child handle
  auto extract_child = []<class E>(const E& e) -> H {
    if constexpr (std::convertible_to<E, H>) {
      return static_cast<H>(e);
    } else {
      return e.target();
    }
  };
  
  // Tarjan's DFS to find SCCs and back-edges
  std::function<void(H)> tarjan_dfs = [&](H node) {
    key_t k = node.stable_key();
    
    state[k] = State::Visiting;
    dfs_num[k] = low_link[k] = counter++;
    stack.push_back(k);
    on_stack.insert(k);
    handle_of[k] = node;
    
    for (auto const& edge_like : view.children(node)) {
      H child = extract_child(edge_like);
      key_t ck = child.stable_key();
      
      handle_of.try_emplace(ck, child);
      
      auto it = state.find(ck);
      if (it == state.end() || it->second == State::Unvisited) {
        // Tree edge - recurse
        tarjan_dfs(child);
        low_link[k] = std::min(low_link[k], low_link[ck]);
      } else if (it->second == State::Visiting) {
        // Back-edge - cycle detected
        result.has_cycles = true;
        result.back_edges[{k, ck}] = true;
        low_link[k] = std::min(low_link[k], dfs_num[ck]);
      }
      // else: cross or forward edge in visited node
    }
    
    state[k] = State::Visited;
    
    // Found SCC root?
    if (low_link[k] == dfs_num[k]) {
      std::size_t scc_id = result.num_sccs++;
      key_t w;
      do {
        w = stack.back();
        stack.pop_back();
        on_stack.erase(w);
        result.cycle_groups[w] = scc_id;
      } while (w != k);
    }
  };
  
  // Run DFS from all roots
  for (auto const& r : view.roots()) {
    H root = r;
    key_t k = root.stable_key();
    if (state.find(k) == state.end()) {
      tarjan_dfs(root);
    }
  }
  
  return result;
}

/**
 * @brief Compute a DFS-based traversal order that handles cycles.
 *
 * @tparam View A type modeling ::dagir::read_only_dag_view
 * @param view The read-only DAG/DCG view
 * @return std::vector<typename View::handle> A DFS traversal order
 *
 * This function performs a depth-first traversal that works for both
 * DAGs and DCGs. Unlike kahn_topological_order, it does not throw on
 * cycles. Nodes are visited in DFS postorder where possible, with
 * cycle-creating edges handled gracefully.
 *
 * Notes:
 *  - For DAGs, produces a valid topological order (reverse postorder)
 *  - For DCGs, produces a traversal that visits all reachable nodes
 *  - Nodes in cycles are visited according to DFS discovery order
 */
template <dagir::concepts::read_only_dag_view View>
std::vector<typename View::handle> dfs_traversal_order(const View& view) {
  using H = typename View::handle;
  using key_t = std::uint64_t;
  
  std::unordered_set<key_t> visited;
  std::unordered_map<key_t, H> handle_of;
  std::vector<H> order;
  
  // Helper to extract child handle
  auto extract_child = []<class E>(const E& e) -> H {
    if constexpr (std::convertible_to<E, H>) {
      return static_cast<H>(e);
    } else {
      return e.target();
    }
  };
  
  // DFS with postorder collection
  std::function<void(H)> dfs = [&](H node) {
    key_t k = node.stable_key();
    
    if (!visited.insert(k).second) {
      return;  // Already visited
    }
    
    handle_of[k] = node;
    
    // Optionally guard traversal for this node
    if constexpr (requires(const View& v, H hh) { v.start_guard(hh); }) {
      auto guard = view.start_guard(node);
      (void)guard;
    }
    
    // Visit children first (postorder)
    for (auto const& edge_like : view.children(node)) {
      H child = extract_child(edge_like);
      key_t ck = child.stable_key();
      
      if (!visited.count(ck)) {
        dfs(child);
      }
    }
    
    // Add node after visiting children
    order.push_back(node);
  };
  
  // Start DFS from all roots
  for (auto const& r : view.roots()) {
    H root = r;
    key_t k = root.stable_key();
    if (!visited.count(k)) {
      dfs(root);
    }
  }
  
  // Reverse to get topological-like order for DAGs
  std::reverse(order.begin(), order.end());
  
  return order;
}

/**
 * @brief Compute a postorder fold over the DAG reachable from `view.roots()`.
 *
 * The combiner is invoked for every node after its children's results are
 * available. The combiner signature is expected to be invocable as:
 *
 *   R combiner(const View& view, typename View::handle node, std::span<const R> child_results)
 *
 * @tparam View A type modeling ::dagir::read_only_dag_view
 * @tparam R Result type
 * @tparam Combiner Callable type as described above
 * @param view The read-only DAG view
 * @param combiner Callable that reduces children's results into the node's result
 * @return std::unordered_map<std::uint64_t, R> Map from node stable_key() -> folded result
 *
 * Implementation note: we reuse Kahn's algorithm to obtain a topological order,
 * then process nodes in reverse topological order so children are computed first.
 */
template <dagir::concepts::read_only_dag_view View, class R, class Combiner>
auto postorder_fold(const View& view, Combiner combiner) -> std::unordered_map<std::uint64_t, R> {
  using H = typename View::handle;
  using key_t = std::uint64_t;

  auto topo = kahn_topological_order(view);
  std::unordered_map<key_t, R> results;
  results.reserve(topo.size());

  // helper to extract child handle (same as above)
  auto extract_child = []<class E>(const E& e) -> H {
    if constexpr (std::convertible_to<E, H>) {
      return static_cast<H>(e);
    } else {
      return e.target();
    }
  };

  // Process in reverse topological order: children before parents
  for (auto it = topo.rbegin(); it != topo.rend(); ++it) {
    H node = *it;
    key_t k = node.stable_key();

    // collect child results in order observed
    std::vector<R> child_vals;
    for (auto const& edge_like : view.children(node)) {
      H child = extract_child(edge_like);
      key_t ck = child.stable_key();
      auto found = results.find(ck);
      if (found != results.end())
        child_vals.push_back(found->second);
      else
        child_vals.push_back(R{});  // absent->default constructed result
    }

    R res = std::invoke(combiner, view, node, std::span(child_vals));
    results.emplace(k, std::move(res));
  }

  return results;
}

}  // namespace dagir