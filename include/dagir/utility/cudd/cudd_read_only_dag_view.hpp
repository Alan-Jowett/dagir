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

/**
 * @brief Non-owning handle pointing at a CUDD node.
 *
 * This lightweight handle wraps a raw `DdNode*` and provides the
 * `stable_key()` and `debug_address()` accessors required by the
 * `dagir::concepts::node_handle` concept.
 */
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

/**
 * @brief Read-only DAG view adapter for CUDD BDD nodes.
 *
 * Non-owning adapter that exposes CUDD diagrams via the
 * `dagir::concepts::read_only_dag_view` concept. The caller must ensure
 * the underlying `DdManager` and node pointers remain valid for the
 * lifetime of this view.
 */
class cudd_read_only_dag_view {
 public:
  using handle = cudd_handle;

  /**
   * @brief Lightweight edge wrapper exposing a child handle.
   */
  struct cudd_edge {
    handle to;
    /**
     * @brief Return the child handle stored in this edge wrapper.
     */
    constexpr const handle& target() const noexcept { return to; }
  };

  /**
   * @brief Construct a view over the given CUDD manager and optional roots.
   *
   * @param mgr Pointer to the CUDD `DdManager` (may be nullptr for empty view).
   * @param var_names Optional pointer to variable name vector owned by caller.
   * @param roots Optional list of root `DdNode*` pointers for the view.
   */
  explicit cudd_read_only_dag_view(DdManager* mgr = nullptr,
                                   const std::vector<std::string>* var_names = nullptr,
                                   std::vector<DdNode*> roots = {})
      : mgr_(mgr), var_names_(var_names), roots_(std::move(roots)) {}

  /**
   * @brief Return pointer to the optional variable name vector.
   *
   * The returned pointer is non-owning and may be nullptr if no names were
   * provided by the caller.
   */
  constexpr const std::vector<std::string>* var_names() const noexcept { return var_names_; }

  /**
   * @brief Return the children (false/true) of the given handle.
   *
   * Returns an empty vector for null or terminal (constant) nodes. For
   * complemented pointers the complement bit is propagated to the
   * returned child handles so the semantic children are observed.
   */
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

  /**
   * @brief Return the configured roots of the view as a vector of handles.
   *
   * If the view has no manager or no roots configured an empty vector
   * is returned.
   */
  auto roots() const {
    if (!mgr_ || roots_.empty()) return std::vector<handle>{};
    std::vector<handle> out;
    out.reserve(roots_.size());
    std::transform(roots_.begin(), roots_.end(), std::back_inserter(out),
                   [](auto r) { return handle{r}; });
    return out;
  }

  /**
   * @brief Return a no-op guard for adapters that do not require pinning.
   *
   * This function satisfies `build_ir`'s optional `start_guard` hook.
   */
  static auto start_guard(const handle&) { return dagir::noop_guard{}; }

 private:
  DdManager* mgr_ = nullptr;
  const std::vector<std::string>* var_names_ = nullptr;
  std::vector<DdNode*> roots_;
};

}  // namespace utility
}  // namespace dagir
