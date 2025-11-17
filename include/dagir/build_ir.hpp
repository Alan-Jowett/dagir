// SPDX-License-Identifier: MIT
// Copyright (c) DagIR Contributors
//
// File: build_ir.hpp
// Brief: Build a renderer-neutral IR (nodes/edges/attributes) from a ReadOnlyDagView.
// Note : Header-only, C++20. Depends on dagir/algorithms.hpp for traversal order.

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <type_traits>
#include <ranges>

#include <dagir/ro_dag_view.hpp> // ReadOnlyDagView
#include <dagir/algorithms.hpp> // topo_order / topo_order_strict and RoDagViewLike
#include <dagir/ir.hpp>

namespace dagir {

/**
 * @brief Extract a child handle from an edge-like range element.
 *
 * @tparam H Target handle type.
 * @tparam E Element type returned from View::children(handle).
 * @param e Edge-like element (either a handle or an EdgeRef with target()).
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
 * @brief Construct an IRGraph from a read-only DAG view.
 *
 * @tparam View A type modeling ::dagir::ReadOnlyDagView
 * @tparam NodeLabeler Callable used to produce node labels. Supported
 *         signatures: `node_label(view, handle)` or `node_label(handle)`.
 * @tparam EdgeAttributor Callable used to produce per-edge attributes.
 *         Supported signatures include:
 *           - `edge_attr(view, parent, edge_like)`
 *           - `edge_attr(view, parent, child_handle)`
 *           - `edge_attr(parent, edge_like)`
 *           - `edge_attr(parent, child_handle)`
 *
 * @param view Read-only DAG view to traverse.
 * @param node_label Node label policy.
 * @param edge_attr Edge attribute policy.
 * @return IRGraph The constructed intermediate representation.
 *
 * Behavior:
 *  - Traverses the DAG in topological order (using `kahn_topological_order`).
 *  - Calls `view.start_guard(handle)` if provided by the adapter.
 *  - Memoizes nodes by `stable_key()` to avoid duplicates.
 */
template <ReadOnlyDagView View, class NodeLabeler, class EdgeAttributor>
IRGraph build_ir(const View& view, NodeLabeler&& node_label, EdgeAttributor&& edge_attr) {
	using H = typename View::handle;
	using key_t = std::uint64_t;

	IRGraph graph;

	// Get a deterministic traversal order (topological for DAGs). We traverse
	// nodes in topological order and generate edges as we go.
	std::vector<H> topo = kahn_topological_order(view);

	graph.nodes.reserve(topo.size());

	// Map stable_key -> node index in graph.nodes
	std::unordered_map<key_t, std::size_t> node_index;
	node_index.reserve(topo.size() * 2);

	// First, create nodes (memoized) using label policy
	for (const H& h : topo) {
		key_t k = h.stable_key();

		// Optionally guard traversal for this node
		if constexpr (requires (const View& v, H hh) { v.start_guard(hh); }) {
			auto guard = view.start_guard(h);
			(void)guard;
		}

		IRNode n;
		n.id = k;

		if constexpr (std::invocable<NodeLabeler, const View&, const H&>) {
			n.label = std::invoke(node_label, view, h);
		} else if constexpr (std::invocable<NodeLabeler, const H&>) {
			n.label = std::invoke(node_label, h);
		} else {
			n.label = std::to_string(k);
		}

		node_index.emplace(k, graph.nodes.size());
		graph.nodes.push_back(std::move(n));
	}

	// Now collect edges; reserve approximate size by summing child counts
	std::size_t est_edges = 0;
	for (const H& h : topo) {
		for (auto const &e : view.children(h)) {
            ++est_edges;
            (e);
        }
	}
	graph.edges.reserve(est_edges);

	for (const H& parent : topo) {
		key_t pk = parent.stable_key();
		for (auto const &edge_like : view.children(parent)) {
			H child = build_ir_extract_child<H>(edge_like);
			key_t ck = child.stable_key();

			IREdge ie;
			ie.source = pk;
			ie.target = ck;

			// Determine attributes via flexible invocation forms
			if constexpr (std::invocable<EdgeAttributor, const View&, const H&, const decltype(edge_like)&>) {
				ie.attributes = std::invoke(edge_attr, view, parent, edge_like);
			} else if constexpr (std::invocable<EdgeAttributor, const View&, const H&, const H&>) {
				ie.attributes = std::invoke(edge_attr, view, parent, child);
			} else if constexpr (std::invocable<EdgeAttributor, const H&, const decltype(edge_like)&>) {
				ie.attributes = std::invoke(edge_attr, parent, edge_like);
			} else if constexpr (std::invocable<EdgeAttributor, const H&, const H&>) {
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
 * @brief Convenience overload that builds an IRGraph using default policies.
 *
 * Default node label: `std::to_string(handle.stable_key())`.
 * Default edge attributes: empty vector.
 */
template <ReadOnlyDagView View>
IRGraph build_ir(const View& view) {
	auto node_label = [](auto const& h) -> std::string { return std::to_string(h.stable_key()); };
	auto edge_attr = [](auto..., auto...) -> std::vector<IRAttr> { return {}; };
	return build_ir(view, node_label, edge_attr);
}

} // namespace dagir
