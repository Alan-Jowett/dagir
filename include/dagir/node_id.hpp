// SPDX-License-Identifier: MIT
// Copyright (c) DagIR Contributors

#pragma once

#include <cstdint>
#include <format>
#include <mutex>
#include <string>
#include <unordered_map>

namespace dagir {
namespace utility {

/**
 * @brief Return a compact unique node id for a stable key.
 *
 * This helper assigns sequential identifiers (node000, node001, ...) for
 * keys seen during program execution. It is thread-safe and intended for
 * use by policy implementations that need renderer-visible unique ids.
 */
inline std::string make_node_id(std::uint64_t key) {
  static std::mutex m;
  static std::unordered_map<std::uint64_t, int> map;
  static int next = 0;
  std::scoped_lock lk(m);
  auto it = map.find(key);
  if (it != map.end()) return std::format("node{:03}", it->second);
  int id = next++;
  map.emplace(key, id);
  return std::format("node{:03}", id);
}

}  // namespace utility
}  // namespace dagir
