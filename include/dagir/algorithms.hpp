#pragma once
// SPDX-License-Identifier: MIT
// © DagIR Contributors. All rights reserved.

#include <functional>
#include <queue>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "dagir/ro_dag_view.hpp"

namespace dagir {

/**
 * @brief Compute a topological ordering of nodes reachable from `view.roots()`
 *        using Kahn's algorithm.
 *
 * @tparam View A type modeling ::dagir::ReadOnlyDagView
 * @param view The read-only DAG view
 * @return std::vector<typename View::handle> A topological ordering of handles.
 * @throws std::runtime_error if a cycle is detected in the reachable subgraph.
 *
 * Notes:
 *  - This function traverses the reachable subgraph starting from `view.roots()`.
 *  - Nodes are identified by their `stable_key()` for hash maps, and the returned
 *    handles preserve the adapter's handle values.
 */
template <ReadOnlyDagView View>
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
 * @brief Compute a postorder fold over the DAG reachable from `view.roots()`.
 *
 * The combiner is invoked for every node after its children's results are
 * available. The combiner signature is expected to be invocable as:
 *
 *   R combiner(const View& view, typename View::handle node, std::span<const R> child_results)
 *
 * @tparam View A type modeling ::dagir::ReadOnlyDagView
 * @tparam R Result type
 * @tparam Combiner Callable type as described above
 * @param view The read-only DAG view
 * @param combiner Callable that reduces children's results into the node's result
 * @return std::unordered_map<std::uint64_t, R> Map from node stable_key() -> folded result
 *
 * Implementation note: we reuse Kahn's algorithm to obtain a topological order,
 * then process nodes in reverse topological order so children are computed first.
 */
template <ReadOnlyDagView View, class R, class Combiner>
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