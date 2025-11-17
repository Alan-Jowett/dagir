// SPDX-License-Identifier: MIT
// Core concept: NodeHandle

#pragma once

#include <concepts>
#include <cstdint>

namespace dagir::concepts {

/**
 * @brief Opaque, cheap handle to a node in a foreign DAG.
 *
 * A type models ::dagir::NodeHandle when:
 *  - It is std::copyable
 *  - It exposes stable_key() returning a std::uint64_t suitable for memoization
 *  - It exposes debug_address() returning a const void* (may be nullptr)
 *  - It supports equality/inequality with identity semantics
 */
template <class H>
concept node_handle = std::copyable<H> && requires(const H& h) {
  { h.stable_key() } -> std::convertible_to<std::uint64_t>;
  { h.debug_address() } -> std::convertible_to<const void*>;
  { h == h } -> std::same_as<bool>;
  { h != h } -> std::same_as<bool>;
};

}  // namespace dagir::concepts
