/**
 * @file build_ir.hpp
 * @brief Build a renderer-neutral IR (nodes, edges, attributes) from a
 *        `read_only_dag_view` adapter.
 *
 * This header provides the `build_ir` utility which accepts a view and
 * policy callables to produce a `dagir::ir_graph`. Policies are expected to
 * be attribute-producing (see `dagir::concepts::node_attributor` and
 * `dagir::concepts::edge_attributor`). The implementation is header-only
 * and uses C++20 concepts and utilities.
 *
 * SPDX-License-Identifier: MIT
 * Copyright (c) DagIR Contributors
 */

#pragma once

#include <cstdint>
#include <dagir/algorithms.hpp>                // kahn_topological_order
#include <dagir/concepts/node_attributor.hpp>  // node_attributor (accept attribute-producing policies)
#include <dagir/concepts/read_only_dag_view.hpp>  // read_only_dag_view
#include <dagir/ir.hpp>
#include <dagir/ir_attrs.hpp>
#include <format>
#include <functional>
#include <numeric>
#include <ranges>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace dagir {

// Detection idiom helpers
//
// NOTE: These lightweight detection traits are retained for historical
// reasons and for use by static analysis tools. `build_ir` requires
// node attributors that model `dagir::concepts::node_attributor`. Attributors
// are expected to produce attribute representations (the canonical type is
// `dagir::ir_attr_map`).
namespace build_ir_detail {
template <class, class = void>
struct has_first_second : std::false_type {};

template <class T>
struct has_first_second<
    T, std::void_t<decltype(std::declval<T>().first), decltype(std::declval<T>().second)>>
    : std::true_type {};

template <class, class = void>
struct has_name_label : std::false_type {};

template <class T>
struct has_name_label<
    T, std::void_t<decltype(std::declval<T>().name), decltype(std::declval<T>().label)>>
    : std::true_type {};
}  // namespace build_ir_detail

/**
 * @brief Extract a child handle from an edge-like range element.
 *
 * @tparam H Target handle type.
 * @tparam E Element type returned from View::children(handle).
 * @param e Edge-like element (either a handle or an edge_ref with target()).
 * @return H The extracted child handle.
 *
 * @note This helper supports adapters that either yield child handles
 * directly or small edge-reference types exposing `target()`.
 */
template <class H, class E>
static H build_ir_extract_child(const E& e) {
  if constexpr (std::convertible_to<E, H>) {
    return static_cast<H>(e);
  } else {
    return e.target();
  }
}

/**
 * @brief Construct an `ir_graph` from a read-only DAG view.
 *
 * @tparam View A type modeling ::dagir::read_only_dag_view
 * @tparam NodePolicy Callable used to produce node attributes. Supported
 *         attributor signatures (produce `dagir::ir_attr_map`):
 *         `node_attr(view, handle)` or `node_attr(handle)`.
 *         `build_ir` requires a `dagir::concepts::node_attributor` — a
 *         callable that returns a `dagir::ir_attr_map` for a given node.
 * @tparam edge_attributor Callable used to produce per-edge attributes.
 *         Supported signatures include:
 *           - `edge_attr(view, parent, edge_like)`
 *           - `edge_attr(view, parent, child_handle)`
 *           - `edge_attr(parent, edge_like)`
 *           - `edge_attr(parent, child_handle)`
 *
 * @param view Read-only DAG view to traverse.
 * @param node_policy Node attributor policy (callable returning attributes).
 * @param edge_attr Edge attribute policy.
 * @return ir_graph The constructed intermediate representation.
 *
 * Behavior:
 *  - Traverses the DAG in topological order (using `kahn_topological_order`).
 *  - Calls `view.start_guard(handle)` if provided by the adapter.
 *  - Memoizes nodes by `stable_key()` to avoid duplicates.
 */
template <dagir::concepts::read_only_dag_view View, class NodePolicy, class EdgePolicy>
  requires dagir::concepts::node_attributor<NodePolicy, View>
ir_graph build_ir(const View& view, NodePolicy&& node_policy, EdgePolicy&& edge_attr) {
  using H = typename View::handle;
  using key_t = std::uint64_t;

  ir_graph graph;

  // Get a deterministic traversal order (topological for DAGs). We traverse
  // nodes in topological order and generate edges as we go.
  std::vector<H> topo = kahn_topological_order(view);

  graph.nodes.reserve(topo.size());

  // First, create nodes (memoized) using label policy
  for (std::size_t idx = 0; idx < topo.size(); ++idx) {
    const H& h = topo[idx];
    key_t k = h.stable_key();

    // Optionally guard traversal for this node
    if constexpr (requires(const View& v, H hh) { v.start_guard(hh); }) {
      auto guard = view.start_guard(h);
      (void)guard;
    }

    ir_node n;
    n.id = k;

    // Default canonical name assigned in topological order; policies must
    // be node-attributors producing `dagir::ir_attr_map` that will populate
    // `n.attributes`. We prefer attribute-provided values; otherwise the
    // default name is used and a label from the stable key is written.
    n.attributes = std::invoke(node_policy, view, h);
    if (!n.attributes.count(std::string_view{"name"}))
      n.attributes[std::string_view{"name"}] = std::format("node{:03}", idx);
    if (!n.attributes.count(ir_attrs::k_label)) n.attributes[ir_attrs::k_label] = std::to_string(k);

    graph.nodes.push_back(std::move(n));
  }

  // Now collect edges; reserve approximate size by summing child counts
  // Reserve an approximate size for edges by summing child counts using
  // standard algorithms. Using std::accumulate makes the intent clearer
  // and satisfies cppcheck's `useStlAlgorithm` suggestion.
  std::size_t est_edges = std::accumulate(
      topo.begin(), topo.end(), std::size_t{0}, [&view](std::size_t acc, const H& h) {
        return acc + static_cast<std::size_t>(std::ranges::distance(view.children(h)));
      });
  graph.edges.reserve(est_edges);

  for (const H& parent : topo) {
    key_t pk = parent.stable_key();
    for (auto const& edge_like : view.children(parent)) {
      H child = build_ir_extract_child<H>(edge_like);
      key_t ck = child.stable_key();

      ir_edge ie;
      ie.source = pk;
      ie.target = ck;

      // Determine attributes via flexible invocation forms
      if constexpr (std::invocable<EdgePolicy, const View&, const H&, const decltype(edge_like)&>) {
        ie.attributes = std::invoke(edge_attr, view, parent, edge_like);
      } else if constexpr (std::invocable<EdgePolicy, const View&, const H&, const H&>) {
        ie.attributes = std::invoke(edge_attr, view, parent, child);
      } else if constexpr (std::invocable<EdgePolicy, const H&, const decltype(edge_like)&>) {
        ie.attributes = std::invoke(edge_attr, parent, edge_like);
      } else if constexpr (std::invocable<EdgePolicy, const H&, const H&>) {
        ie.attributes = std::invoke(edge_attr, parent, child);
      } else {
        ie.attributes = {};
      }

      graph.edges.push_back(std::move(ie));
    }
  }

  return graph;
}

/**
 * @brief Convenience overload that builds an `ir_graph` using default policies.
 *
 * Default node label: `std::to_string(handle.stable_key())`.
 * Default edge attributes: empty vector.
 */
template <dagir::concepts::read_only_dag_view View>
ir_graph build_ir(const View& view) {
  auto node_attr = [](auto const& /*view*/, auto const& h) -> dagir::ir_attr_map {
    dagir::ir_attr_map m;
    m.emplace(ir_attrs::k_label, std::format("{}", h.stable_key()));
    return m;
  };
  auto edge_attr = [](auto&&...) -> dagir::ir_attr_map { return {}; };
  return build_ir(view, node_attr, edge_attr);
}

}  // namespace dagir
