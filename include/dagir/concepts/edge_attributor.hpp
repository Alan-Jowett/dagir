// SPDX-License-Identifier: MIT
// Concept: edge_attributor
// Defines the `dagir::concepts::edge_attributor` concept used by policy implementers.

#pragma once

#include <concepts>
#include <type_traits>
#include <utility>

namespace dagir {

/**
 * @brief Concept for an edge attribute policy callable.
 *
 * @tparam F Callable type.
 * @tparam View A type that models ::dagir::read_only_dag_view.
 *
 * @details
 * A type models ::dagir::edge_attributor when an lvalue of @c F is invocable as:
 *  - @c f(view, parent_handle, child_handle)
 *
 * The return type is unconstrained (string, struct, map, etc.) and is left to the
 * renderer/IR builder to interpret. Edge attribution is optional.
 */
template <class F, class View>
concept edge_attributor = requires(const F& f, const View& v, const typename View::handle& p,
                                   const typename View::handle& c) {
  { f(v, p, c) };
};

}  // namespace dagir
