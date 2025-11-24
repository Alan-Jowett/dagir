/*
 * @file ir_serialization.cpp
 * @brief Minimal example of IR serialization usage.
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025 DagIR contributors
 */

#include <dagir/ir.hpp>
#include <dagir/ir_serialization.hpp>
#include <iostream>

int main() {
  dagir::ir_graph g;

  dagir::ir_node n;
  n.id = 1;
  n.attributes[g.attr_cache.cache_view(dagir::ir_attrs::k_name)] = g.attr_cache.cache_view("root");
  n.attributes[g.attr_cache.cache_view(dagir::ir_attrs::k_label)] =
      g.attr_cache.cache_view("Root Node");
  g.nodes.push_back(std::move(n));

  try {
    std::string s = dagir::serialize::to_json(g);
    std::cout << s << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "serialization not available: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
