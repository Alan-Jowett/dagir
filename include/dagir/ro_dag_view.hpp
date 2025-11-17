// SPDX-License-Identifier: MIT
//
// DagIR - Read-only external DAG view concepts and helpers
// A lightweight interface for traversing foreign DAGs without copying.
//
// © DagIR Contributors. All rights reserved.

#pragma once

#include <concepts>
#include <cstdint>
#include <ranges>
#include <string>
#include <type_traits>
#include <utility>

namespace dagir {

/**
 * @file ro_dag_view.hpp
 * @brief Core concepts and small utilities for creating read-only external DAG views.
 *
 * The contracts in this header let algorithms (e.g., topological order, post‑order
 * folds, IR builders) traverse *foreign* DAGs without copying or owning the data.
 * Adapters implement these concepts on top of backends like TeDDy, CUDD, or
 * any domain DAG (build graphs, expression DAGs, workflow DAGs, etc.).
 */

//-----------------------------
// 1) Core element concepts (split into one header per concept)
//-----------------------------

#include <dagir/concepts/children_range.hpp>
#include <dagir/concepts/edge_ref.hpp>
#include <dagir/concepts/node_handle.hpp>

//-----------------------------------------------
// 2) Read-only External DAG View concept
//-----------------------------------------------

/**
 * @brief Read-only, non-owning view over a foreign DAG.
 *
 * @tparam G Adapter/view type.
 *
 * @details
 * A type models ::dagir::ReadOnlyDagView when:
 *  - It defines @c using handle = ... ; and that type models ::dagir::NodeHandle
 *  - It provides @c children(handle) returning a ::dagir::ChildrenRange of edges
 *  - It provides @c roots() returning an @c input_range of @c handle values
 *
 * Optional:
 *  - @c start_guard(handle) returning an RAII guard object (e.g., to pin nodes,
 *    disable reordering, or otherwise enforce traversal safety). Adapters that
 *    do not require any guarding may return ::dagir::NoopGuard.
 */
template <class G>
concept ReadOnlyDagView = requires(const G& g, typename G::handle h) {
  typename G::handle;
  requires NodeHandle<typename G::handle>;
  { g.children(h) } -> ChildrenRange<typename G::handle>;
  { g.roots() } -> std::ranges::input_range;
  // Optional: g.start_guard(h)
};

//-----------------------------------------------
// 3) Lightweight adapter utilities (optional)
//-----------------------------------------------

/**
 * @brief No-op RAII guard for adapters that do not require pinning/reordering locks.
 *
 * @details
 * Adapters can @c using guard_type = ::dagir::NoopGuard and implement
 * @c start_guard(handle) returning a @c NoopGuard to satisfy uniform call sites.
 */
struct NoopGuard {
  NoopGuard() = default;
  ~NoopGuard() = default;
  NoopGuard(const NoopGuard&) = delete;
  NoopGuard& operator=(const NoopGuard&) = delete;
  NoopGuard(NoopGuard&&) = default;
  NoopGuard& operator=(NoopGuard&&) = default;
};

/**
 * @brief Compile-time probe: returns @c true if @p V models ::dagir::ReadOnlyDagView.
 *
 * @tparam V Candidate view type.
 * @return constexpr @c bool
 *
 * @usage
 * @code
 * static_assert(dagir::models_read_only_view<MyView>(), "MyView must model ReadOnlyDagView");
 * @endcode
 */
template <class V>
consteval bool models_read_only_view() {
  if constexpr (ReadOnlyDagView<V>)
    return true;
  else
    return false;
}

//-----------------------------------------------
// 4) Example: minimal edge wrapper (for adapters)
//-----------------------------------------------

/**
 * @brief Trivial edge wrapper that stores a child handle by value.
 *
 * @tparam H Handle type that models ::dagir::NodeHandle.
 *
 * @details
 * Adapters may reuse this when they do not need custom edge labels/weights.
 */
template <NodeHandle H>
struct BasicEdge {
  H to;  ///< Child handle
  /// @brief Returns the child handle.
  constexpr const H& target() const noexcept { return to; }
};

// Policy concept helpers live in their own header so implementations can
// check conformance without pulling in the entire build_ir algorithm.
#include <dagir/concepts/edge_attributor.hpp>
#include <dagir/concepts/node_labeler.hpp>

}  // namespace dagir
