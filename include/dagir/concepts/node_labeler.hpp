// SPDX-License-Identifier: MIT
// Concept: node_labeler
// Defines the `dagir::concepts::node_labeler` concept used by policy implementers.

#pragma once

#include <concepts>
#include <functional>
#include <string>

namespace dagir::concepts {

/**
 * @brief Concept for a node labeling policy callable.
 *
 * @tparam F Callable type.
 * @tparam View A type that models ::dagir::read_only_dag_view.
 *
 * @details
 * A type models ::dagir::node_labeler when an lvalue of @c F is invocable as:
 *  - @c f(view, handle) and returns a @c std::string
 *
 * This lets renderers/IR-builders fetch labels without coupling to adapter internals.
 */
template <class F, class View>
concept node_labeler = requires(const F& f, const View& v, const typename View::handle& h) {
  { f(v, h) } -> std::convertible_to<std::string>;
};

}  // namespace dagir::concepts
