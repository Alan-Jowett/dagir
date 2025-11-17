// SPDX-License-Identifier: MIT
// Core concept: edge_ref

#pragma once

#include <concepts>

namespace dagir::concepts {

template <class E, class H>
concept edge_ref = requires(const E& e) {
  { e.target() } -> std::convertible_to<H>;
};

}  // namespace dagir::concepts
