// SPDX-License-Identifier: MIT
/**
 * @file
 * @brief Concept to describe an edge reference type.
 *
 * This header declares the `edge_ref` concept which requires that a type
 * `E` provide a `target()` accessor convertible to a handle type `H`.
 * It is used to generically accept edge-like objects returned by graph
 * views and iterators.
 */

#pragma once

#include <concepts>

namespace dagir::concepts {

/**
 * @concept edge_ref
 * @tparam E The candidate edge-reference type to test.
 * @tparam H The handle type the edge should expose via `target()`.
 * @brief True if `E` exposes a `target()` convertible to `H`.
 *
 * Typical implementations of `E` are lightweight proxy objects returned when
 * iterating the adjacency or children of a node. This concept intentionally
 * keeps requirements minimal (just a `target()` accessor) so it is easy to
 * satisfy with different underlying graph implementations.
 */
template <class E, class H>
concept edge_ref = requires(const E& e) {
  { e.target() } -> std::convertible_to<H>;
};

}  // namespace dagir::concepts
