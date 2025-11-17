// SPDX-License-Identifier: MIT
// Concept: EdgeAttributor
// Defines the `dagir::concepts::EdgeAttributor` concept used by policy implementers.

#pragma once

#include <concepts>
#include <type_traits>
#include <utility>

namespace dagir {

/**
 * @brief Concept for an edge attribute policy callable.
 *
 * A type `F` models `edge_attributor<F, View>` when it is invocable in at least
 * one of the supported forms. The return type is unconstrained here to keep
 * the concept flexible; the `build_ir` algorithm converts the result to
 * `std::vector<dagir::ir_attr>` where appropriate.
 *
 * Supported forms include but are not limited to:
 *  - `f(view, parent_handle, edge_like)`
 *  - `f(view, parent_handle, child_handle)`
 *  - `f(parent_handle, edge_like)`
 *  - `f(parent_handle, child_handle)`
 */
template <class F, class View>
concept edge_attributor = requires(const F& f, const View& v, const typename View::handle& p,
                                   const typename View::handle& c, const typename View::handle& e) {
  { std::invoke(f, v, p, e) } -> std::same_as<void>;
} || requires(const F& f, const View& v, const typename View::handle& p) {
  { std::invoke(f, v, p, c) } -> std::same_as<void>;
} || requires(const F& f, const typename View::handle& p, const typename View::handle& e) {
  { std::invoke(f, p, e) } -> std::same_as<void>;
} || requires(const F& f, const typename View::handle& p) {
  { std::invoke(f, p, c) } -> std::same_as<void>;
};

}  // namespace dagir
