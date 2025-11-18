// SPDX-License-Identifier: MIT
/**
 * @file
 * @brief Read-only DAG view concept and small helper types.
 *
 * The `read_only_dag_view` concept describes an adapter that provides
 * read-only access to a DAG-like structure: it must expose a nested
 * `handle` type, a `children(handle)` accessor returning an iterable of
 * edges, and a `roots()` accessor returning an input range of root handles.
 *
 * This header also provides a trivial `noop_guard` RAII type for adapters
 * that do not require locking, a compile-time probe `models_read_only_view`,
 * and a `basic_edge` helper that satisfies the `edge_ref` concept.
 */

#pragma once

#include <concepts>
#include <dagir/concepts/children_range.hpp>
#include <dagir/concepts/node_handle.hpp>
#include <ranges>

namespace dagir::concepts {

/**
 * @concept read_only_dag_view
 * @tparam G Candidate view type.
 * @brief True if `G` offers read-only DAG accessors.
 *
 * Requirements:
 *  - `G` must declare a nested `handle` type modeling `node_handle`.
 *  - `g.children(h)` must return a range of edges modeling
 *    `children_range<typename G::handle>` for a handle `h`.
 *  - `g.roots()` must return an input range of root handles.
 */
template <class G>
concept read_only_dag_view = requires(const G& g, typename G::handle h) {
  typename G::handle;
  requires node_handle<typename G::handle>;
  { g.children(h) } -> children_range<typename G::handle>;
  { g.roots() } -> std::ranges::input_range;
};

}  // namespace dagir::concepts

namespace dagir {

/**
 * @brief No-op RAII guard for adapters that do not require pinning or locks.
 *
 * Adapters that need to pin or lock internal resources can provide a guard
 * type; for adapters that do not, `noop_guard` provides a trivial replacement.
 */
struct noop_guard {
  noop_guard() = default;
  ~noop_guard() = default;
  noop_guard(const noop_guard&) = delete;
  noop_guard& operator=(const noop_guard&) = delete;
  noop_guard(noop_guard&&) = default;
  noop_guard& operator=(noop_guard&&) = default;
};

/**
 * @brief Compile-time probe returning true if `V` models `read_only_dag_view`.
 *
 * This helper is intended for `static_assert` or test-time checks.
 */
template <class V>
consteval bool models_read_only_view() {
  if constexpr (concepts::read_only_dag_view<V>)
    return true;
  else
    return false;
}

/**
 * @brief Minimal edge wrapper storing a child handle by value.
 *
 * `basic_edge` satisfies the `edge_ref` concept by exposing a
 * `target()` accessor that returns the stored handle.
 */
template <concepts::node_handle H>
struct basic_edge {
  H to;
  constexpr const H& target() const noexcept { return to; }
};

}  // namespace dagir
