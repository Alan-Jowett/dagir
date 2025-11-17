// SPDX-License-Identifier: MIT
// Concept: EdgeAttributor
// Defines the `dagir::concepts::EdgeAttributor` concept used by policy implementers.

#pragma once

#include <concepts>
#include <type_traits>
#include <utility>

namespace dagir::concepts {

/**
 * @brief Concept for an edge attribute policy callable.
 *
 * A type `F` models `EdgeAttributor<F, View>` when it is invocable in at least
 * one of the supported forms. The return type is unconstrained here to keep
 * the concept flexible; the `build_ir` algorithm converts the result to
 * `std::vector<dagir::IRAttr>` where appropriate.
 *
 * Supported forms include but are not limited to:
 *  - `f(view, parent_handle, edge_like)`
 *  - `f(view, parent_handle, child_handle)`
 *  - `f(parent_handle, edge_like)`
 *  - `f(parent_handle, child_handle)`
 */
template <class F, class View>
concept EdgeAttributor =
    requires(const F& f, const View& v, const typename View::handle& p,
             const typename View::handle& c, const auto& edge_like) {
      { std::invoke(f, v, p, edge_like) };
    } ||
    requires(const F& f, const View& v, const typename View::handle& p,
             const typename View::handle& c) {
      { std::invoke(f, v, p, c) };
    } ||
    requires(const F& f, const typename View::handle& p, const auto& edge_like) {
      { std::invoke(f, p, edge_like) };
    } || requires(const F& f, const typename View::handle& p, const typename View::handle& c) {
      { std::invoke(f, p, c) };
    };

}  // namespace dagir::concepts
