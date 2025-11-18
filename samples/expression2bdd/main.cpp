/**
 * @file main.cpp
 * @brief Sample CLI: parse expression, convert to BDD, render IR via DagIR
 *
 * Usage: expression2bdd <expr_file> <library> <backend>
 *   library: teddy
 *   backend: dot | json | mermaid
 *
 * SPDX-License-Identifier: MIT
 */

#include <dagir/build_ir.hpp>
#include <dagir/render_dot.hpp>
#include <dagir/render_json.hpp>
#include <dagir/render_mermaid.hpp>
#include <iostream>
#include <set>
#include <string>

#include "expressions/expression_parser.hpp"

// Teddy-specific helpers
#include "teddy/teddy_convert_expression.hpp"
#include "teddy/teddy_policy.hpp"
#include "teddy/teddy_read_only_dag_view.hpp"

int main(int argc, char** argv) {
  using namespace dagir::utility;

  if (argc < 4) {
    std::cerr << "Usage: " << argv[0] << " <expression_file> <library> <backend>\n";
    std::cerr << "library: teddy\n";
    std::cerr << "backend: dot | json | mermaid\n";
    return 1;
  }

  const std::string filename = argv[1];
  const std::string library = argv[2];
  const std::string backend = argv[3];

  try {
    my_expression_ptr expr = read_expression_from_file(filename);

    if (library == "teddy") {
      // Create a Teddy manager and convert expression to a diagram
      // Use DagIR algorithms to collect variable names from the expression AST.
      std::unordered_map<std::string, int> var_map;
      {
        // Build a read-only view over the expression AST and run a postorder_fold
        // combiner that collects variable names into an unordered_set.
        using vec_t = std::vector<std::string>;
        dagir::utility::expression_read_only_dag_view expr_view(expr.get());

        auto results = dagir::postorder_fold<dagir::utility::expression_read_only_dag_view, vec_t>(
            expr_view,
            [](auto const& /*view*/, auto node, std::span<const vec_t> child_vecs) -> vec_t {
              vec_t out;
              // Concatenate child vectors in left-to-right order
              for (auto const& v : child_vecs) out.insert(out.end(), v.begin(), v.end());

              // If this node is a variable, append its name
              if (auto p_var = std::get_if<my_variable>(node.ptr)) {
                out.push_back(p_var->variable_name);
              }

              return out;
            });

        // Build var_map preserving first occurrence order across the root(s)
        std::unordered_set<std::string> seen;
        int idx = 0;
        for (auto const& r : expr_view.roots()) {
          auto it = results.find(r.stable_key());
          if (it == results.end()) continue;
          for (auto const& name : it->second) {
            if (seen.insert(name).second) {
              var_map.emplace(name, idx++);
            }
          }
        }

        // Create manager with variable count and a reasonable node pool size
      }

      teddy::bdd_manager mgr(static_cast<int32_t>(var_map.size()), 1024);

      // Build inverse map: index -> name for labeling variable nodes
      std::vector<std::string> var_names;
      var_names.resize(var_map.size());
      for (auto const& [name, idx] : var_map) {
        if (idx >= 0 && static_cast<size_t>(idx) < var_names.size())
          var_names[static_cast<size_t>(idx)] = name;
      }

      auto diag = convert_expression_to_teddy(mgr, *expr, var_map);

      // Extract root pointer
      std::vector<teddy::bdd_manager::diagram_t::node_t*> roots;
      roots.push_back(diag.unsafe_get_root());

      dagir::utility::teddy_read_only_dag_view view(&mgr, &var_names, std::move(roots));

      // Build IR using teddy policies
      dagir::ir_graph ir = dagir::build_ir(view, dagir::utility::teddy_node_attributor{},
                                           dagir::utility::teddy_edge_attributor{});

      // Render
      if (backend == "dot") {
        dagir::render_dot(std::cout, ir, "bdd");
      } else if (backend == "json") {
        dagir::render_json(std::cout, ir);
      } else if (backend == "mermaid") {
        std::cout << "```mermaid\n";
        dagir::render_mermaid(std::cout, ir, "bdd");
        std::cout << "```\n";
      } else {
        std::cerr << "Unknown backend: " << backend << "\n";
        return 1;
      }

    } else {
      std::cerr << "Unsupported library: " << library << "\n";
      return 1;
    }

  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }

  return 0;
}
