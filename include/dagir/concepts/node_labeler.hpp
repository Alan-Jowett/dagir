// SPDX-License-Identifier: MIT
// Concept: NodeLabeler
// Defines the `dagir::concepts::NodeLabeler` concept used by policy implementers.

#pragma once

#include <concepts>
#include <functional>
#include <string>

namespace dagir::concepts {

/**
 * @brief Concept for a node labeling policy callable.
 *
 * A type `F` models `NodeLabeler<F, View>` when it is invocable in one of the
 * supported forms and the result is convertible to `std::string`:
 *  - `f(view, handle)`
 *  - `f(handle)`
 *
 * The `View` parameter is expected to be a type that models a DagIR view
 * (provides `using handle = ...`). This concept is intentionally permissive
 * to match the overloads supported by the IR builder.
 */
template <class F, class View>
concept NodeLabeler = (requires(const F& f, const View& v, const typename View::handle& h) {
                        { std::invoke(f, v, h) } -> std::convertible_to<std::string>;
                      }) || (requires(const F& f, const typename View::handle& h) {
                        { std::invoke(f, h) } -> std::convertible_to<std::string>;
                      });

}  // namespace dagir::concepts
