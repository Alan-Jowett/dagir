/**
 * @file expression_read_only_dag_view.hpp
 * @brief Read-only DAG view for expression ASTs.
 *
 * @details
 *
 * @copyright
 * © DagIR Contributors. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <array>
#include <cstdint>
#include <dagir/concepts/read_only_dag_view.hpp>
#include <dagir/utility/expressions/expression_ast.hpp>
#include <type_traits>
#include <utility>
#include <vector>

// Implement read_only_dag_view over expression ASTs
namespace dagir {
namespace utility {
/**
 * @brief Non-owning handle pointing at a node inside a parsed expression AST.
 *
 * This handle wraps a pointer into an expression AST and provides the
 * stable_key/debug_address accessors required by the DAG view concepts.
 */
struct expression_handle {
  const my_expression* ptr = nullptr;

  // Use non-constexpr stable_key since pointer-to-integer casts are not
  // permitted in a constexpr context on all compilers.
  std::uint64_t stable_key() const noexcept {
    return static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(ptr));
  }

  constexpr const void* debug_address() const noexcept { return static_cast<const void*>(ptr); }

  constexpr bool operator==(const expression_handle& o) const noexcept { return ptr == o.ptr; }
  constexpr bool operator!=(const expression_handle& o) const noexcept { return ptr != o.ptr; }
};

/**
 * @brief Read-only adapter exposing an expression AST as a DAG view.
 *
 * Non-owning: the caller must ensure the lifetime of the root expression
 * passed to the constructor.
 */
class expression_read_only_dag_view {
 public:
  using handle = expression_handle;

  /**
   * @brief Lightweight edge type for this adapter satisfying `edge_ref`.
   */
  struct expression_edge {
    handle to;
    /** Return the target handle stored in this edge wrapper. */
    constexpr const handle& target() const noexcept { return to; }
  };

  /**
   * @brief Construct a view over the provided expression AST root.
   *
   * @param root Pointer to the root expression (non-owning).
   */
  explicit expression_read_only_dag_view(const my_expression* root = nullptr) : root_{root} {}

  /**
   * @brief Return children (edges) of the given handle.
   *
   * Returns a vector of `expression_edge` representing each child. The
   * vector is empty for null handles or leaf nodes.
   */
  auto children(const handle& h) const {
    std::vector<expression_edge> out;
    if (!h.ptr) return out;

    if (auto p_and = std::get_if<my_and>(h.ptr)) {
      if (p_and->left) out.push_back(expression_edge{handle{p_and->left.get()}});
      if (p_and->right) out.push_back(expression_edge{handle{p_and->right.get()}});
    } else if (auto p_or = std::get_if<my_or>(h.ptr)) {
      if (p_or->left) out.push_back(expression_edge{handle{p_or->left.get()}});
      if (p_or->right) out.push_back(expression_edge{handle{p_or->right.get()}});
    } else if (auto p_xor = std::get_if<my_xor>(h.ptr)) {
      if (p_xor->left) out.push_back(expression_edge{handle{p_xor->left.get()}});
      if (p_xor->right) out.push_back(expression_edge{handle{p_xor->right.get()}});
    } else if (auto p_not = std::get_if<my_not>(h.ptr)) {
      if (p_not->expr) out.push_back(expression_edge{handle{p_not->expr.get()}});
    }

    return out;
  }

  /**
   * @brief Return the set of roots for this view (one-element or empty).
   */
  auto roots() const {
    if (!root_) return std::vector<handle>{};
    return std::vector<handle>{handle{root_}};
  }

  /**
   * @brief No-op guard for this in-memory view.
   */
  static auto start_guard(const handle&) { return dagir::noop_guard{}; }

 private:
  const my_expression* root_ = nullptr;
};

}  // namespace utility
}  // namespace dagir