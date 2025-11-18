// SPDX-License-Identifier: MIT
/**
 * @file
 * @brief Concept to detect ranges of child edge references.
 *
 * This header declares the `children_range` concept which is satisfied by
 * input ranges whose value type or reference type models `edge_ref` for a
 * given handle type `H`. It is used by generic algorithms that iterate
 * over the children (outgoing edges) of a node in the IR.
 */

#pragma once

#include <dagir/concepts/edge_ref.hpp>
#include <ranges>
#include <type_traits>

namespace dagir::concepts {

/**
 * @concept children_range
 * @tparam R The range type to check (must satisfy `std::ranges::input_range`).
 * @tparam H The handle type used by edges (for example a node identifier).
 * @brief True if `R` is a range of `edge_ref<..., H>`-compatible elements.
 *
 * The concept accepts either a range whose value type models `edge_ref<..., H>`
 * or a range whose reference type (after removing cvref) models `edge_ref<..., H>`.
 * This allows it to match both owning ranges and ranges of references/views.
 */
template <class R, class H>
concept children_range = std::ranges::input_range<R> &&
                         (edge_ref<std::ranges::range_value_t<R>, H> ||
                          edge_ref<std::remove_cvref_t<std::ranges::range_reference_t<R>>, H>);

}  // namespace dagir::concepts
