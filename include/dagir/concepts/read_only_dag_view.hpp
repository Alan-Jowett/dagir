// SPDX-License-Identifier: MIT
// Core concept: ReadOnlyDagView

#pragma once

#include <concepts>
#include <dagir/concepts/children_range.hpp>
#include <dagir/concepts/node_handle.hpp>
#include <ranges>

namespace dagir {

template <class G>
concept read_only_dag_view = requires(const G& g, typename G::handle h) {
  typename G::handle;
  requires node_handle<typename G::handle>;
  { g.children(h) } -> children_range<typename G::handle>;
  { g.roots() } -> std::ranges::input_range;
};

}  // namespace dagir
