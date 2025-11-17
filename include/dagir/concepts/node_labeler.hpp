// SPDX-License-Identifier: MIT
// Concept: node_labeler
// Defines the `dagir::concepts::node_labeler` concept used by policy implementers.

#pragma once

#include <concepts>
#include <functional>
#include <string>

namespace dagir::concepts {

template <class F, class View>
concept node_labeler_with_view =
    requires(const F& f, const View& v, const typename View::handle& h) {
      { f(v, h) } -> std::convertible_to<std::string>;
    };

template <class F, class View>
concept node_labeler_with_handle = requires(const F& f, const typename View::handle& h) {
  { f(h) } -> std::convertible_to<std::string>;
};

/**
 * @brief Concept for a node labeling policy callable.
 *
 * @tparam F Callable type.
 * @tparam View A type that models ::dagir::concepts::read_only_dag_view.
 *
 * @details
 * A type models ::dagir::concepts::node_labeler when an lvalue of @c F is
 * invocable as either:
 *  - @c f(view, handle), or
 *  - @c f(handle),
 * and the result is convertible to @c std::string.
 *
 * This lets renderers/IR-builders fetch labels without coupling to adapter internals.
 */
template <class F, class View>
concept node_labeler = node_labeler_with_view<F, View> || node_labeler_with_handle<F, View>;

}  // namespace dagir::concepts
