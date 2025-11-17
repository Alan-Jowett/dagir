// SPDX-License-Identifier: MIT
// Core concept: ChildrenRange

#pragma once

#include <dagir/concepts/edge_ref.hpp>
#include <ranges>
#include <type_traits>

namespace dagir {

template <class R, class H>
concept children_range = std::ranges::input_range<R> &&
                         (edge_ref<std::ranges::range_value_t<R>, H> ||
                          edge_ref<std::remove_cvref_t<std::ranges::range_reference_t<R>>, H>);

}  // namespace dagir
