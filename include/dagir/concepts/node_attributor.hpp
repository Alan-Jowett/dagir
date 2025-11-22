/**
 * @file node_attributor.hpp
 * @brief Concept for node attribute-producing policy callables.
 *
 * Policies modeling `dagir::concepts::node_attributor` are callables that
 * accept a `read_only_dag_view` and a node handle and return an attribute
 * representation (commonly `dagir::ir_attr_map`). The return type is
 * intentionally unconstrained to allow flexibility.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <concepts>

#include "dagir/concepts/name_value_range.hpp"

namespace dagir::concepts {
// Helper concepts are defined in `name_value_range.hpp`.

/**
 * @brief Concept for a node attribute policy callable.
 *
 * @tparam F Callable type.
 * @tparam View A type that models `dagir::concepts::read_only_dag_view`.
 *
 * @details
 * A type models `dagir::concepts::node_attributor` when an lvalue of @c F is
 * invocable as:
 *  - @c f(view, node_handle)
 *
 * The callable must return a forward range of name/value pairs where both
 * the key (`.first`) and value (`.second`) of each element are convertible
 * to `std::string_view`. This models name/value attribute maps used by the
 * renderer/IR builder (for example, containers of `std::pair` or
 * associative containers exposing element-like references).
 */
template <class F, class View>
concept node_attributor = requires(const F& f, const View& v, const typename View::handle& n) {
  { f(v, n) } -> name_value_range;
};

}  // namespace dagir::concepts
