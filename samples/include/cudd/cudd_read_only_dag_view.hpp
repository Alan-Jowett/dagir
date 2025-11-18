/**
 * @file cudd_read_only_dag_view.hpp
 * @brief Read-only DAG view for CUDD BDDs.
 *
 * @details
 *  Non-owning adapter exposing CUDD BDD nodes as a DagIR read-only view.
 *
 * @copyright
 * © DagIR Contributors. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cudd/cudd.h>

#include <algorithm>
#include <cstdint>
#include <dagir/concepts/read_only_dag_view.hpp>
#include <string>
#include <vector>

namespace dagir {
namespace utility {

// Non-owning handle pointing at a CUDD node
struct cudd_handle {
  using node_ptr = DdNode*;
  node_ptr ptr = nullptr;

  std::uint64_t stable_key() const noexcept {
    return static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(ptr));
  }

  constexpr const void* debug_address() const noexcept { return static_cast<const void*>(ptr); }
  constexpr bool operator==(const cudd_handle& o) const noexcept { return ptr == o.ptr; }
  constexpr bool operator!=(const cudd_handle& o) const noexcept { return ptr != o.ptr; }
};

class cudd_read_only_dag_view {
 public:
  using handle = cudd_handle;

  struct cudd_edge {
    handle to;
    constexpr const handle& target() const noexcept { return to; }
  };

  explicit cudd_read_only_dag_view(DdManager* mgr = nullptr,
                                   const std::vector<std::string>* var_names = nullptr,
                                   std::vector<DdNode*> roots = {})
      : mgr_(mgr), var_names_(var_names), roots_(std::move(roots)) {}

  constexpr const std::vector<std::string>* var_names() const noexcept { return var_names_; }

  static auto children(const handle& h) {
    std::vector<cudd_edge> out;
    if (!h.ptr) return out;

    if (Cudd_IsConstant(h.ptr)) return out;

    // Handle possibly complemented node pointers. Use regular node to read
    // children, then propagate the complement bit to the returned children
    // so the rest of the code sees semantically-correct pointers.
    const bool is_comp = Cudd_IsComplement(h.ptr);
    DdNode* base = Cudd_Regular(h.ptr);

    // else (0) then (1) ordering
    DdNode* else_child = Cudd_E(base);
    DdNode* then_child = Cudd_T(base);

    if (is_comp) {
      if (else_child) else_child = Cudd_Not(else_child);
      if (then_child) then_child = Cudd_Not(then_child);
    }

    if (else_child) out.push_back(cudd_edge{handle{else_child}});
    if (then_child) out.push_back(cudd_edge{handle{then_child}});

    return out;
  }

  auto roots() const {
    if (!mgr_ || roots_.empty()) return std::vector<handle>{};
    std::vector<handle> out;
    out.reserve(roots_.size());
    std::transform(roots_.begin(), roots_.end(), std::back_inserter(out),
                   [](auto r) { return handle{r}; });
    return out;
  }

  static auto start_guard(const handle&) { return dagir::noop_guard{}; }

 private:
  DdManager* mgr_ = nullptr;
  const std::vector<std::string>* var_names_ = nullptr;
  std::vector<DdNode*> roots_;
};

}  // namespace utility
}  // namespace dagir
