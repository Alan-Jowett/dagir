/**
 * @file teddy_read_only_dag_view.hpp
 * @brief Read-only DAG view for TeDDy BDDs.
 *
 * @details
 *
 * @copyright
 * © DagIR Contributors. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "libteddy/core.hpp"

// Implement read_only_dag_view over TeDDy BDDs
#include <algorithm>
#include <cstdint>
#include <dagir/concepts/read_only_dag_view.hpp>
#include <string>
#include <vector>

namespace dagir {
namespace utility {

// Non-owning handle pointing at a TeDDy node
/**
 * @brief Lightweight non-owning handle to a TeDDy node.
 *
 * Models `dagir::concepts::node_handle` for use with DagIR adapters.
 */
struct teddy_handle {
  using node_ptr = teddy::bdd_manager::diagram_t::node_t*;
  node_ptr ptr = nullptr;

  /**
   * @brief Stable key for memoization.
   *
   * Terminals are encoded using their logical value to avoid pointer-based
   * collisions; non-terminals use pointer identity.
   */
  std::uint64_t stable_key() const noexcept {
    return static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(ptr));
  }

  /**
   * @brief Debug address for diagnostics (may be nullptr).
   */
  constexpr const void* debug_address() const noexcept { return static_cast<const void*>(ptr); }

  /**
   * @brief Identity comparison for handles.
   */
  constexpr bool operator==(const teddy_handle& o) const noexcept { return ptr == o.ptr; }
  constexpr bool operator!=(const teddy_handle& o) const noexcept { return ptr != o.ptr; }
};

/**
 * @brief Read-only adapter exposing a TeDDy BDD diagram as a DAG view.
 *
 * Non-owning: the caller is responsible for the lifetime of the
 * `teddy::bdd_manager` and the underlying diagram nodes.
 */
class teddy_read_only_dag_view {
 public:
  using handle = teddy_handle;

  /**
   * @brief Lightweight edge type carrying the child handle.
   */
  struct teddy_edge {
    handle to;
    /** @brief Return the child handle target. */
    constexpr const handle& target() const noexcept { return to; }
  };

  /**
   * @brief Construct a read-only view over a TeDDy diagram.
   *
   * @param mgr Pointer to the TeDDy bdd_manager that owns the diagram.
   * @param var_names Optional variable name array for labeling.
   * @param roots Optional list of root node pointers for the view.
   */
  explicit teddy_read_only_dag_view(teddy::bdd_manager* mgr = nullptr,
                                    const std::vector<std::string>* var_names = nullptr,
                                    std::vector<teddy::bdd_manager::diagram_t::node_t*> roots = {})
      : mgr_(mgr), var_names_(var_names), roots_(std::move(roots)) {}

  /**
   * @brief Optional variable name array previously supplied to the view.
   * @return pointer to the variable name vector or nullptr if not provided.
   */
  constexpr const std::vector<std::string>* var_names() const noexcept { return var_names_; }

  /**
   * @brief Return the outgoing edges for a handle (false then true).
   *
   * Returns an empty vector for terminal nodes or null handles.
   */
  static auto children(const handle& h) {
    std::vector<teddy_edge> out;
    if (!h.ptr) return out;

    if (h.ptr->is_terminal()) return out;

    auto false_child = h.ptr->get_son(0);
    auto true_child = h.ptr->get_son(1);

    if (false_child) out.push_back(teddy_edge{handle{false_child}});
    if (true_child) out.push_back(teddy_edge{handle{true_child}});

    return out;
  }

  /**
   * @brief Return the list of root handles for this view.
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
   * @brief Start guard used by traversal algorithms; noop for this adapter.
   */
  static auto start_guard(const handle&) { return dagir::noop_guard{}; }

 private:
  teddy::bdd_manager* mgr_ = nullptr;
  const std::vector<std::string>* var_names_ = nullptr;
  std::vector<teddy::bdd_manager::diagram_t::node_t*> roots_;
};

}  // namespace utility
}  // namespace dagir
