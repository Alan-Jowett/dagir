// SPDX-License-Identifier: MIT
/**
 * @file
 * @brief Concepts describing node labeling callables.
 *
 * This header provides concepts that describe callables usable as node
 * labeler policies. Implementations may accept either the view+handle
 * pair or just a node handle; both forms must return a value convertible
 * to `std::string`.
 */

#pragma once

#include <concepts>
#include <functional>
#include <string>

namespace dagir::concepts {

/**
 * @concept node_labeler_with_view
 * @tparam F Callable type.
 * @tparam View A type that models `dagir::concepts::read_only_dag_view`.
 * @brief True if `F` is invocable as `f(view, handle)` and returns a string.
 */
template <class F, class View>
concept node_labeler_with_view =
    requires(const F& f, const View& v, const typename View::handle& h) {
      { f(v, h) } -> std::convertible_to<std::string>;
    };

/**
 * @concept node_labeler_with_handle
 * @tparam F Callable type.
 * @tparam View A type that models `dagir::concepts::read_only_dag_view`.
 * @brief True if `F` is invocable as `f(handle)` and returns a string.
 */
template <class F, class View>
concept node_labeler_with_handle = requires(const F& f, const typename View::handle& h) {
  { f(h) } -> std::convertible_to<std::string>;
};

/**
 * @concept node_labeler
 * @tparam F Callable type.
 * @tparam View A type that models `dagir::concepts::read_only_dag_view`.
 * @brief True if `F` can be used to obtain a node label for a view.
 *
 * A callable `F` models `node_labeler` if it is invocable in either of the
 * forms documented above and the result is convertible to `std::string`.
 */
template <class F, class View>
concept node_labeler = node_labeler_with_view<F, View> || node_labeler_with_handle<F, View>;

}  // namespace dagir::concepts
