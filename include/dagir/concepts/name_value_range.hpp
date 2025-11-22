/**
 * @file name_value_range.hpp
 * @brief Helper concepts for name/value attribute ranges.
 *
 * Provides `string_view_convertible`, `name_value_element`, and
 * `name_value_range` used by node and edge attributor concepts.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <concepts>
#include <ranges>
#include <string_view>
#include <type_traits>
#include <utility>

namespace dagir::concepts {

/**
 * @brief Test whether a type is convertible to `std::string_view`.
 *
 * @tparam T candidate type
 *
 * This concept is used by the name/value helpers to ensure keys and values
 * can be treated as string-like views without requiring ownership.
 */
template <class T>
concept string_view_convertible = std::convertible_to<T, std::string_view>;

/**
 * @brief Checks that a range element exposes name/value members.
 *
 * @tparam R range-like type whose value type should expose `.first` and
 *           `.second` members.
 *
 * The element type (after removing cv/ref) must provide accessible
 * `.first` and `.second` members and both must be convertible to
 * `std::string_view`.
 */
template <class R>
concept name_value_element = requires {
  typename std::remove_cvref_t<std::ranges::range_value_t<R>>;
} && requires(std::remove_cvref_t<std::ranges::range_value_t<R>>& val) {
  { val.first } -> string_view_convertible;
  { val.second } -> string_view_convertible;
};

/**
 * @brief A forward-range whose elements are name/value pairs.
 *
 * @tparam R range type
 *
 * This concept requires a `std::ranges::forward_range` and that each
 * element satisfy `name_value_element` (i.e., exposes `.first` and
 * `.second` convertible to `std::string_view`). This models attribute maps and
 * containers used by renderers and IR builders.
 */
template <class R>
concept name_value_range = std::ranges::forward_range<R> && name_value_element<R>;

}  // namespace dagir::concepts
