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

// CUDD-specific helpers
#include "cudd/cudd_convert_expression.hpp"
#include "cudd/cudd_policy.hpp"
#include "cudd/cudd_read_only_dag_view.hpp"

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

      // Sort IR nodes and edges deterministically by the printable node name
      auto node_print_name = [&](const dagir::ir_node& n) {
        auto it = n.attributes.find("name");
        if (it != n.attributes.end()) return it->second;
        return std::to_string(n.id);
      };

      std::sort(ir.nodes.begin(), ir.nodes.end(),
                [&](const dagir::ir_node& a, const dagir::ir_node& b) {
                  return node_print_name(a) < node_print_name(b);
                });

      // Rebuild a mapping from old id to new id (if we preserved ids, keep them the same)
      // The renderer expects node ids to remain unique; here we leave numeric ids untouched
      // but sorting ensures emission order is stable.

      std::sort(ir.edges.begin(), ir.edges.end(),
                [&](const dagir::ir_edge& A, const dagir::ir_edge& B) {
                  std::string a_src, a_dst, b_src, b_dst;
                  // find node names for A
                  auto fa = std::find_if(ir.nodes.begin(), ir.nodes.end(),
                                         [&](const dagir::ir_node& n) { return n.id == A.source; });
                  if (fa != ir.nodes.end())
                    a_src = node_print_name(*fa);
                  else
                    a_src = std::to_string(A.source);
                  auto ta = std::find_if(ir.nodes.begin(), ir.nodes.end(),
                                         [&](const dagir::ir_node& n) { return n.id == A.target; });
                  if (ta != ir.nodes.end())
                    a_dst = node_print_name(*ta);
                  else
                    a_dst = std::to_string(A.target);
                  // find node names for B
                  auto fb = std::find_if(ir.nodes.begin(), ir.nodes.end(),
                                         [&](const dagir::ir_node& n) { return n.id == B.source; });
                  if (fb != ir.nodes.end())
                    b_src = node_print_name(*fb);
                  else
                    b_src = std::to_string(B.source);
                  auto tb = std::find_if(ir.nodes.begin(), ir.nodes.end(),
                                         [&](const dagir::ir_node& n) { return n.id == B.target; });
                  if (tb != ir.nodes.end())
                    b_dst = node_print_name(*tb);
                  else
                    b_dst = std::to_string(B.target);

                  if (a_src != b_src) return a_src < b_src;
                  return a_dst < b_dst;
                });

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

    } else if (library == "cudd") {
      std::unordered_map<std::string, int> var_map;
      {
        using vec_t = std::vector<std::string>;
        dagir::utility::expression_read_only_dag_view expr_view(expr.get());

        auto results = dagir::postorder_fold<dagir::utility::expression_read_only_dag_view, vec_t>(
            expr_view,
            [](auto const& /*view*/, auto node, std::span<const vec_t> child_vecs) -> vec_t {
              vec_t out;
              for (auto const& v : child_vecs) out.insert(out.end(), v.begin(), v.end());
              if (auto p_var = std::get_if<my_variable>(node.ptr)) {
                out.push_back(p_var->variable_name);
              }
              return out;
            });

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
      }

      // Initialize CUDD manager
      DdManager* mgr =
          Cudd_Init(static_cast<int>(var_map.size()), 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);

      // Build inverse map: index -> name for labeling variable nodes
      std::vector<std::string> var_names;
      var_names.resize(var_map.size());
      for (auto const& [name, idx] : var_map) {
        if (idx >= 0 && static_cast<size_t>(idx) < var_names.size())
          var_names[static_cast<size_t>(idx)] = name;
      }

      DdNode* diag = convert_expression_to_cudd(*mgr, *expr, var_map);

      std::vector<DdNode*> roots;
      roots.push_back(diag);

      dagir::utility::cudd_read_only_dag_view view(mgr, &var_names, std::move(roots));

      dagir::ir_graph ir = dagir::build_ir(view, dagir::utility::cudd_node_attributor{},
                                           dagir::utility::cudd_edge_attributor{});

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
        Cudd_RecursiveDeref(mgr, diag);
        Cudd_Quit(mgr);
        return 1;
      }

      // Clean up CUDD objects
      Cudd_RecursiveDeref(mgr, diag);
      Cudd_Quit(mgr);

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
