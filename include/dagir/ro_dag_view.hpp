// SPDX-License-Identifier: MIT
//
// DagIR - Read-only external DAG view concepts and helpers
// A lightweight interface for traversing foreign DAGs without copying.
//
// © DagIR Contributors. All rights reserved.

#pragma once

#include <concepts>
#include <dagir/concepts/children_range.hpp>
#include <dagir/concepts/edge_attributor.hpp>
#include <dagir/concepts/edge_ref.hpp>
#include <dagir/concepts/node_handle.hpp>
#include <dagir/concepts/node_labeler.hpp>
#include <ranges>

namespace dagir {

/**
 * @brief Read-only DAG view concept and small adapter utilities.
 */

template <class G>
concept read_only_dag_view = requires(const G& g, typename G::handle h) {
  typename G::handle;
  requires node_handle<typename G::handle>;
  { g.children(h) } -> children_range<typename G::handle>;
  { g.roots() } -> std::ranges::input_range;
};

/// No-op RAII guard for adapters that do not require pinning/reordering locks.
struct noop_guard {
  noop_guard() = default;
  ~noop_guard() = default;
  noop_guard(const noop_guard&) = delete;
  noop_guard& operator=(const noop_guard&) = delete;
  noop_guard(noop_guard&&) = default;
  noop_guard& operator=(noop_guard&&) = default;
};

/// Compile-time probe: returns true if V models read_only_dag_view.
template <class V>
consteval bool models_read_only_view() {
  if constexpr (read_only_dag_view<V>)
    return true;
  else
    return false;
}

/// Minimal edge wrapper storing a child handle by value.
template <node_handle H>
struct basic_edge {
  H to;
  constexpr const H& target() const noexcept { return to; }
};

}  // namespace dagir
