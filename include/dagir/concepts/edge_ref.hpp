// SPDX-License-Identifier: MIT
// Core concept: EdgeRef

#pragma once

#include <concepts>

namespace dagir {

template <class E, class H>
concept edge_ref = requires(const E& e) {
  { e.target() } -> std::convertible_to<H>;
};

}  // namespace dagir
