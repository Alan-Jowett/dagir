/**
 * @file edge_attributor.hpp
 * @brief Concept for edge attribute-producing policy callables.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <concepts>
#include <type_traits>
#include <utility>

#include "dagir/concepts/name_value_range.hpp"

namespace dagir::concepts {

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
 * The callable must return a forward range of name/value pairs where both
 * the key (`.first`) and value (`.second`) of each element are convertible
 * to `std::string_view`. This matches the refinement applied to
 * `dagir::concepts::node_attributor` and models attribute maps used by the
 * renderer/IR builder.
 */
template <class F, class View>
concept edge_attributor = requires(const F& f, const View& v, const typename View::handle& p,
                                   const typename View::handle& c) {
  { f(v, p, c) } -> name_value_range;
};

}  // namespace dagir::concepts
