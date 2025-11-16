#pragma once
// SPDX-License-Identifier: MIT
//
// DagIR - Read-only external DAG view concepts and helpers
// A lightweight interface for traversing foreign DAGs without copying.

#include <concepts>
#include <cstdint>
#include <ranges>
#include <type_traits>
#include <utility>

namespace dagir {

//-----------------------------
// 1) Core element concepts
//-----------------------------

/// Opaque, cheap handle to a node in a foreign DAG.
/// Identity must be stable for the lifetime of a traversal.
template <class H>
concept NodeHandle =
    std::copyable<H> &&
    requires (const H& h) {
        // 64-bit stable key usable for memo tables, maps, sets, etc.
        { h.stable_key() }    -> std::convertible_to<std::uint64_t>;
        // Optional debug hook (may return nullptr)
        { h.debug_address() } -> std::convertible_to<const void*>;
        // Identity equality (same logical node; phase should be included by the handle if relevant)
        { h == h }            -> std::same_as<bool>;
        { h != h }            -> std::same_as<bool>;
    };

/// A lightweight edge reference yielding a child handle.
/// Labels/weights are optional and can be added by adapters if needed.
template <class E, class H>
concept EdgeRef =
    requires (const E& e) {
        { e.target() } -> std::convertible_to<H>;
        // Optional:
        // { e.label() } -> /* anything */;
    };

// Ranges-v3-like compatibility: accept either value-type or reference-type edge models.
template <class R, class H>
concept ChildrenRange =
    std::ranges::input_range<R> &&
    ( EdgeRef<std::ranges::range_value_t<R>, H> ||
      EdgeRef<std::remove_cvref_t<std::ranges::range_reference_t<R>>, H> );

//-----------------------------------------------
// 2) Read-only External DAG View concept
//-----------------------------------------------

/// A read-only view over a foreign DAG (no ownership, no mutation).
/// Requirements deliberately small: handle type + children() + roots().
template <class G>
concept ReadOnlyDagView =
    requires (const G& g, typename G::handle h) {
        // Handle type
        typename G::handle; requires NodeHandle<typename G::handle>;

        // Outgoing edges of a node (read-only). Range of edge refs yielding handles.
        { g.children(h) } -> ChildrenRange<typename G::handle>;

        // Roots of the subgraph represented by this view (can be empty if caller supplies roots).
        { g.roots() } -> std::ranges::input_range;

        // Optional no-op guard for backends that need pinning/critical sections.
        // Adapters can provide: auto guard = g.start_guard(h); (RAII type)
    };

//-----------------------------------------------
// 3) Lightweight adapter utilities (optional)
//-----------------------------------------------

/// A default, no-op guard for adapters that don't need pinning/reordering control.
/// Adapters can `using guard_type = NoopGuard;` and return it from start_guard().
struct NoopGuard {
    NoopGuard() = default;
    ~NoopGuard() = default;
    NoopGuard(const NoopGuard&) = delete;
    NoopGuard& operator=(const NoopGuard&) = delete;
    NoopGuard(NoopGuard&&) = default;
    NoopGuard& operator=(NoopGuard&&) = default;
};

/// A tiny helper to check at compile-time that a type models ReadOnlyDagView.
/// Usage: static_assert(dagir::models_read_only_view<MyView>());
template <class V>
consteval bool models_read_only_view() {
    if constexpr (ReadOnlyDagView<V>) return true;
    else return false;
}

//-----------------------------------------------
// 4) Example: minimal edge wrapper (for adapters)
//-----------------------------------------------
// Adapters can reuse this if they just need a simple E{H} edge type.

template <NodeHandle H>
struct BasicEdge {
    H to;
    constexpr const H& target() const noexcept { return to; }
};

//-----------------------------------------------
// 5) Example: policy hooks (labels, attributes)
//-----------------------------------------------
// These are *not* part of the core concept, but serve as extension points
// that renderers/IR builders can depend on without forcing any backend shape.

/// Node labeler policy callable: F(const View&, handle) -> std::string
template <class F, class View>
concept NodeLabeler =
    requires (const F& f, const View& v, const typename View::handle& h) {
        { f(v, h) } -> std::convertible_to<std::string>;
    };

/// Edge attribute policy callable (optional):
/// F(const View&, parent_handle, child_handle) -> attribute struct/string/map
template <class F, class View>
concept EdgeAttributor =
    requires (const F& f, const View& v,
              const typename View::handle& p,
              const typename View::handle& c) {
        { f(v, p, c) };
    };

} // namespace dagir
