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
 * The return type is unconstrained and left to the renderer/IR builder to interpret.
 */
template <class F, class View>
concept edge_attributor = requires(const F& f, const View& v, const typename View::handle& p,
                                   const typename View::handle& c) {
  { f(v, p, c) };
};

}  // namespace dagir::concepts
