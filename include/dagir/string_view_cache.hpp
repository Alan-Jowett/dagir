/**
 * @file string_view_cache.hpp
 * @brief String view cache utility for string view lifetime management.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <list>
#include <string>
#include <string_view>
#include <unordered_set>

namespace dagir {

/**
 * @brief A stable string storage/cache for obtaining `std::string_view`
 * references that remain valid for the lifetime of the cache.
 *
 * Implementation notes:
 * - Strings are stored in a `std::list<std::string>` so that addresses of
 *   stored characters remain stable across insertions.
 * - An `std::unordered_set<std::string_view>` indexes the stored strings for
 *   fast lookup. The set stores `string_view` copies that refer into the
 *   list storage.
 */
class string_view_cache {
 public:
  std::string_view cache_view(const std::string_view& sv) {
    auto it = index_.find(sv);
    if (it != index_.end()) return *it;

    storage_.push_back(std::string(sv));
    std::string_view view(storage_.back());
    index_.insert(view);
    return view;
  }

  // Make the cache movable but not copyable. Copying would leave
  // std::string_view entries in `index_` pointing at the original
  // `storage_` and would produce dangling views in the copy.
  string_view_cache() = default;
  string_view_cache(string_view_cache&& other) noexcept : storage_(std::move(other.storage_)) {
    // Rebuild index to point into our own storage
    rebuild_index_from_storage();
  }

  string_view_cache& operator=(string_view_cache&& other) noexcept {
    if (this != &other) {
      storage_ = std::move(other.storage_);
      // Rebuild index to point into the moved-in storage
      rebuild_index_from_storage();
    }
    return *this;
  }

  string_view_cache(const string_view_cache&) = delete;
  string_view_cache& operator=(const string_view_cache&) = delete;

 private:
  std::list<std::string> storage_;
  std::unordered_set<std::string_view> index_;

  void rebuild_index_from_storage() {
    index_.clear();
    for (const auto& s : storage_) index_.insert(std::string_view(s));
  }
};

}  // namespace dagir
