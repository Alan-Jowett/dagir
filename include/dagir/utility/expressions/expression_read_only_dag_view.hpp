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
// Non-owning handle pointing at a node inside a parsed expression AST.
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

/// Read-only adapter exposing an expression AST as a DAG view.
/// Non-owning: the caller must ensure the lifetime of the root expression.
class expression_read_only_dag_view {
 public:
  using handle = expression_handle;

  // Lightweight edge type for this adapter satisfying `edge_ref` concept.
  struct expression_edge {
    handle to;
    constexpr const handle& target() const noexcept { return to; }
  };

  explicit expression_read_only_dag_view(const my_expression* root = nullptr) : root_{root} {}

  // Return a range (vector) of edges for the given handle. Edges carry target handles.
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

  // Return roots as a one-element range (or empty if no root)
  auto roots() const {
    if (!root_) return std::vector<handle>{};
    return std::vector<handle>{handle{root_}};
  }

  // No-op guard for this simple in-memory view
  static auto start_guard(const handle&) { return dagir::noop_guard{}; }

 private:
  const my_expression* root_ = nullptr;
};

}  // namespace utility
}  // namespace dagir