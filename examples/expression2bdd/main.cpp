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

#include <algorithm>
#include <dagir/build_ir.hpp>
#include <dagir/render_dot.hpp>
#include <dagir/render_json.hpp>
#include <dagir/render_mermaid.hpp>
#include <iostream>
#include <set>
#include <span>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Expression parser
#include <dagir/utility/expressions/expression_parser.hpp>

// Teddy-specific helpers
#include <dagir/utility/teddy/teddy_convert_expression.hpp>
#include <dagir/utility/teddy/teddy_policy.hpp>
#include <dagir/utility/teddy/teddy_read_only_dag_view.hpp>

// CUDD-specific helpers
#include <dagir/utility/cudd/cudd_convert_expression.hpp>
#include <dagir/utility/cudd/cudd_policy.hpp>
#include <dagir/utility/cudd/cudd_read_only_dag_view.hpp>

/**
 * @brief Render an IR graph using the requested backend.
 *
 * Sorts nodes and edges deterministically to ensure stable output, then
 * forwards to the selected renderer (`dot`, `json`, or `mermaid`).
 *
 * @param in_ir Input IR graph (copied internally for reordering).
 * @param backend Target backend name: "dot", "json", or "mermaid".
 * @throws std::runtime_error If an unknown backend is requested.
 */
static void emit_ir(const dagir::ir_graph& in_ir, const std::string& backend) {
  dagir::ir_graph ir = in_ir;  // make a local copy we can reorder

  auto node_print_name = [&](const dagir::ir_node& n) {
    auto it = n.attributes.find(dagir::ir_attrs::k_id);
    if (it != n.attributes.end()) return it->second;
    return std::to_string(n.id);
  };

  // Sort nodes by printable name ('name' attribute if present, otherwise id)
  std::sort(
      ir.nodes.begin(), ir.nodes.end(), [&](const dagir::ir_node& a, const dagir::ir_node& b) {
        auto a_it = a.attributes.find(dagir::ir_attrs::k_id);
        auto b_it = b.attributes.find(dagir::ir_attrs::k_id);
        std::string a_name = (a_it != a.attributes.end()) ? a_it->second : std::to_string(a.id);
        std::string b_name = (b_it != b.attributes.end()) ? b_it->second : std::to_string(b.id);
        if (a_name != b_name) return a_name < b_name;
        return a.id < b.id;
      });

  std::sort(ir.edges.begin(), ir.edges.end(),
            [&](const dagir::ir_edge& A, const dagir::ir_edge& B) {
              std::string a_src, a_dst, b_src, b_dst;
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

              // Prefer true-edges (solid) before false-edges (dashed) to match
              // the sample expected ordering used in the repository.
              auto style_of = [&](const dagir::ir_edge& e) -> std::string {
                auto it = e.attributes.find(dagir::ir_attrs::k_style);
                if (it != e.attributes.end()) return it->second;
                return std::string{};
              };

              const std::string a_style = style_of(A);
              const std::string b_style = style_of(B);

              // Straightforward lexicographic ordering: (source, target, style)
              if (a_dst != b_dst) return a_dst < b_dst;
              return a_style < b_style;
            });
  // Sort edges by source node printable name, then target printable name, then style
  auto find_node_name = [&](std::uint64_t id) -> std::string {
    auto it = std::find_if(ir.nodes.begin(), ir.nodes.end(),
                           [&](const dagir::ir_node& n) { return n.id == id; });
    if (it != ir.nodes.end()) {
      auto nit = it->attributes.find(dagir::ir_attrs::k_id);
      if (nit != it->attributes.end()) return nit->second;
      return std::to_string(it->id);
    }
    return std::to_string(id);
  };

  std::sort(ir.edges.begin(), ir.edges.end(),
            [&](const dagir::ir_edge& A, const dagir::ir_edge& B) {
              const std::string a_src = find_node_name(A.source);
              const std::string b_src = find_node_name(B.source);
              if (a_src != b_src) return a_src < b_src;
              const std::string a_tgt = find_node_name(A.target);
              const std::string b_tgt = find_node_name(B.target);
              if (a_tgt != b_tgt) return a_tgt < b_tgt;
              auto a_style_it = A.attributes.find(dagir::ir_attrs::k_style);
              auto b_style_it = B.attributes.find(dagir::ir_attrs::k_style);
              const std::string a_style =
                  (a_style_it != A.attributes.end()) ? a_style_it->second : std::string{};
              const std::string b_style =
                  (b_style_it != B.attributes.end()) ? b_style_it->second : std::string{};
              return a_style < b_style;
            });

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
    throw std::runtime_error("Unknown backend");
  }
}

/**
 * @brief Collect variable names from an expression AST and assign indices.
 *
 * Runs a `postorder_fold` over an expression read-only view, records
 * variable occurrences and assigns small integer indices in first-seen order.
 *
 * @param expr Pointer to the expression variant root.
 * @return Map from variable name to assigned index.
 */
static std::unordered_map<std::string, int> build_var_map(dagir::utility::my_expression* expr) {
  using vec_t = std::vector<std::string>;
  std::unordered_map<std::string, int> var_map;
  dagir::utility::expression_read_only_dag_view expr_view(expr);

  auto results = dagir::postorder_fold<dagir::utility::expression_read_only_dag_view, vec_t>(
      expr_view, [](auto const& /*view*/, auto node, std::span<const vec_t> child_vecs) -> vec_t {
        vec_t out;
        for (auto const& v : child_vecs) out.insert(out.end(), v.begin(), v.end());
        if (auto p_var = std::get_if<dagir::utility::my_variable>(node.ptr)) {
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

  return var_map;
}

/**
 * @brief Build an index -> name vector from a name->index map.
 *
 * This is useful to supply variable names to external libraries that
 * expect a vector indexed by variable index.
 *
 * @param var_map Map from variable name to index.
 * @return Vector where result[idx] == variable_name.
 */
static std::vector<std::string> build_var_names(
    const std::unordered_map<std::string, int>& var_map) {
  std::vector<std::string> var_names;
  var_names.resize(var_map.size());
  for (auto const& [name, idx] : var_map) {
    if (idx >= 0 && static_cast<size_t>(idx) < var_names.size())
      var_names[static_cast<size_t>(idx)] = name;
  }
  return var_names;
}

/**
 * @brief CLI entrypoint for the expression2bdd sample application.
 *
 * Usage: expression2bdd <expression_file> <library> <backend>
 *   library: teddy | cudd
 *   backend: dot | json | mermaid
 */
int main(int argc, char** argv) {
  using namespace dagir::utility;

  if (argc < 4) {
    std::cerr << "Usage: " << argv[0] << " <expression_file> <library> <backend>\n";
    std::cerr << "library: teddy | cudd\n";
    std::cerr << "backend: dot | json | mermaid\n";
    return 1;
  }

  const std::string filename = argv[1];
  const std::string library = argv[2];
  const std::string backend = argv[3];

  try {
    my_expression_ptr expr = read_expression_from_file(filename);

    // Use DagIR algorithms to collect variable names from the expression AST.
    auto var_map = build_var_map(expr.get());
    // Build inverse map (index -> name) once and reuse for both libraries
    auto var_names = build_var_names(var_map);

    if (library == "teddy") {
      // Create a Teddy manager and convert expression to a diagram
      teddy::bdd_manager mgr(static_cast<int32_t>(var_map.size()), 1024);

      auto diag = convert_expression_to_teddy(mgr, *expr, var_map);

      // Extract root pointer
      std::vector<teddy::bdd_manager::diagram_t::node_t*> roots;
      roots.push_back(diag.unsafe_get_root());

      dagir::utility::teddy_read_only_dag_view view(&mgr, &var_names, std::move(roots));

      // Build IR using teddy policies and render deterministically
      dagir::ir_graph ir = dagir::build_ir(view, dagir::utility::teddy_node_attributor{},
                                           dagir::utility::teddy_edge_attributor{});
      emit_ir(ir, backend);

    } else if (library == "cudd") {
      // Initialize CUDD manager
      DdManager* mgr =
          Cudd_Init(static_cast<int>(var_map.size()), 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);

      DdNode* diag = convert_expression_to_cudd(*mgr, *expr, var_map);

      std::vector<DdNode*> roots;
      roots.push_back(diag);

      dagir::utility::cudd_read_only_dag_view view(mgr, &var_names, std::move(roots));

      // Build IR using cudd policies and render deterministically
      dagir::ir_graph ir = dagir::build_ir(view, dagir::utility::cudd_node_attributor{},
                                           dagir::utility::cudd_edge_attributor{});
      try {
        emit_ir(ir, backend);
      } catch (...) {
        Cudd_RecursiveDeref(mgr, diag);
        Cudd_Quit(mgr);
        throw;
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
