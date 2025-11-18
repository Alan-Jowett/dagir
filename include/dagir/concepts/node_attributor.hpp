// SPDX-License-Identifier: MIT
// Concept: node_attributor
// Defines the `dagir::concepts::node_attributor` concept used by policy implementers.

#pragma once

#include <concepts>
#include <type_traits>
#include <utility>

namespace dagir::concepts {

/**
 * @brief Concept for a node attribute policy callable.
 *
 * @tparam F Callable type.
 * @tparam View A type that models ::dagir::read_only_dag_view.
 *
 * @details
 * A type models ::dagir::node_attributor when an lvalue of @c F is invocable as:
 *  - @c f(view, node_handle)
 *
 * The return type is unconstrained (string, struct, map, etc.) and is left to the
 * renderer/IR builder to interpret. Node attribution is optional.
 */
template <class F, class View>
concept node_attributor = requires(const F& f, const View& v, const typename View::handle& n) {
  { f(v, n) };
};

}  // namespace dagir::concepts
