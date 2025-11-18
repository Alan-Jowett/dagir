// SPDX-License-Identifier: MIT
// Copyright (c) DagIR Contributors
//
// File: build_ir.hpp
// Brief: Build a renderer-neutral IR (nodes/edges/attributes) from a read_only_dag_view.
// Note : Header-only, C++20. Depends on dagir/algorithms.hpp for traversal order.

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

// Detection idiom helpers for legacy node labeler return shapes
//
// NOTE: We intentionally use the older "detection idiom" / SFINAE-style
// trait checks here rather than writing the checks inline with C++20
// `requires(...)` expressions that reference member names. Some static
// analysis tools (notably cppcheck) do not understand the newer pattern
// and will report false-positive "redundant code" or "unused member"
// warnings when the `requires` block mentions members without actually
// using them at runtime. Using dedicated detection traits avoids those
// false positives while preserving compile-time detection of supported
// return shapes (pair-like or name/label-like types).
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
 * @tparam node_labeler_or_attributor Callable used to produce node labels or
 *         node attributes. Supported attributor signatures (produce
 *         `dagir::ir_attr_map`): `node_attr(view, handle)` or `node_attr(handle)`.
 *         For backward compatibility this template also accepts labeler
 *         callables that return string-like or pair/name-label shapes.
 * @tparam edge_attributor Callable used to produce per-edge attributes.
 *         Supported signatures include:
 *           - `edge_attr(view, parent, edge_like)`
 *           - `edge_attr(view, parent, child_handle)`
 *           - `edge_attr(parent, edge_like)`
 *           - `edge_attr(parent, child_handle)`
 *
 * @param view Read-only DAG view to traverse.
 * @param node_label Node label policy.
 * @param edge_attr Edge attribute policy.
 * @return ir_graph The constructed intermediate representation.
 *
 * Behavior:
 *  - Traverses the DAG in topological order (using `kahn_topological_order`).
 *  - Calls `view.start_guard(handle)` if provided by the adapter.
 *  - Memoizes nodes by `stable_key()` to avoid duplicates.
 */
template <dagir::concepts::read_only_dag_view View, class NodePolicy, class EdgePolicy>
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

    // Default canonical name assigned in topological order. Policies may
    // override this by returning a name from the node_labeler.
    n.name = std::format("node{}", idx + 1);

    // Flexible node policy return types:
    //  - If the policy returns `dagir::ir_attr_map` it is treated as a
    //    node-attributor and its result populates `n.attributes`.
    //  - Otherwise the policy is interpreted as a node-labeler with the
    //    same supported shapes as before (string, pair<string,string>, or
    //    struct with .name and .label members).
    //
    // The detection order prefers attribute-producing policies when their
    // return type matches `std::vector<ir_attr>`.
    //  - std::string: interpreted as label
    //  - pair<string,string>: interpreted as (name,label)
    //  - struct with .name and .label members: used directly
    if constexpr (std::invocable<NodePolicy, const View&, const H&>) {
      using ret_t = std::invoke_result_t<NodePolicy, const View&, const H&>;
      if constexpr (std::convertible_to<ret_t, dagir::ir_attr_map>) {
        n.attributes = std::invoke(node_policy, view, h);
        if (n.attributes.count("name")) n.name = n.attributes["name"];
        if (n.attributes.count("label")) n.label = n.attributes["label"];
      } else if constexpr (std::convertible_to<ret_t, std::string>) {
        n.label = std::invoke(node_policy, view, h);
      } else if constexpr (build_ir_detail::has_first_second<ret_t>::value &&
                           std::convertible_to<decltype(std::declval<ret_t>().first),
                                               std::string> &&
                           std::convertible_to<decltype(std::declval<ret_t>().second),
                                               std::string>) {
        auto r = std::invoke(node_policy, view, h);
        n.name = std::string(r.first);
        n.label = std::string(r.second);
      } else if constexpr (build_ir_detail::has_name_label<ret_t>::value &&
                           std::convertible_to<decltype(std::declval<ret_t>().name), std::string> &&
                           std::convertible_to<decltype(std::declval<ret_t>().label),
                                               std::string>) {
        auto r = std::invoke(node_policy, view, h);
        n.name = std::string(r.name);
        n.label = std::string(r.label);
      } else {
        n.label = std::to_string(k);
      }
    } else if constexpr (std::invocable<NodePolicy, const H&>) {
      using ret_t = std::invoke_result_t<NodePolicy, const H&>;
      if constexpr (std::convertible_to<ret_t, dagir::ir_attr_map>) {
        n.attributes = std::invoke(node_policy, h);
        if (n.attributes.count("name")) n.name = n.attributes["name"];
        if (n.attributes.count("label")) n.label = n.attributes["label"];
      } else if constexpr (std::convertible_to<ret_t, std::string>) {
        n.label = std::invoke(node_policy, h);
      } else if constexpr (build_ir_detail::has_first_second<ret_t>::value &&
                           std::convertible_to<decltype(std::declval<ret_t>().first),
                                               std::string> &&
                           std::convertible_to<decltype(std::declval<ret_t>().second),
                                               std::string>) {
        auto r = std::invoke(node_policy, h);
        n.name = std::string(r.first);
        n.label = std::string(r.second);
      } else if constexpr (build_ir_detail::has_name_label<ret_t>::value &&
                           std::convertible_to<decltype(std::declval<ret_t>().name), std::string> &&
                           std::convertible_to<decltype(std::declval<ret_t>().label),
                                               std::string>) {
        auto r = std::invoke(node_policy, h);
        n.name = std::string(r.name);
        n.label = std::string(r.label);
      } else {
        n.label = std::to_string(k);
      }
    } else {
      n.label = std::to_string(k);
    }

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
  auto node_label = [](auto const& h) -> std::string { return std::format("{}", h.stable_key()); };
  auto edge_attr = [](auto&&...) -> dagir::ir_attr_map { return {}; };
  return build_ir(view, node_label, edge_attr);
}

}  // namespace dagir
