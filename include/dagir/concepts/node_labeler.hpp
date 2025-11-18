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

// Helper concept describing acceptable node-labeler return shapes.
template <class R>
concept node_label_result =
    std::convertible_to<R, std::string> ||
    (requires(R r) {
      r.first;
      r.second;
    } && std::convertible_to<decltype(std::declval<R>().first), std::string> &&
     std::convertible_to<decltype(std::declval<R>().second), std::string>) ||
    (requires(R r) {
      r.name;
      r.label;
    } && std::convertible_to<decltype(std::declval<R>().name), std::string> &&
     std::convertible_to<decltype(std::declval<R>().label), std::string>);

/**
 * @concept node_labeler_with_view
 * @tparam F Callable type.
 * @tparam View A type that models `dagir::concepts::read_only_dag_view`.
 * @brief True if `F` is invocable as `f(view, handle)` and returns a string.
 */
template <class F, class View>
concept node_labeler_with_view = requires(const F& f, const View& v,
                                          const typename View::handle& h) {
  { f(v, h) };
} && requires {
  requires node_label_result<std::invoke_result_t<F, const View&, const typename View::handle&>>;
};

/**
 * @concept node_labeler_with_handle
 * @tparam F Callable type.
 * @tparam View A type that models `dagir::concepts::read_only_dag_view`.
 * @brief True if `F` is invocable as `f(handle)` and returns a string.
 */
template <class F, class View>
concept node_labeler_with_handle = requires(const F& f, const typename View::handle& h) {
  { f(h) };
} && requires {
  requires node_label_result<std::invoke_result_t<F, const typename View::handle&>>;
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
