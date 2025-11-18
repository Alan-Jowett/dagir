/**
 * @file main.cpp
 * @brief Sample program demonstrating expression parsing to AST, then running it through the DagIR
 * pipeline.
 * @copyright
 * © DagIR Contributors. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

#include <dagir/build_ir.hpp>
#include <dagir/render_dot.hpp>
#include <dagir/render_json.hpp>
#include <dagir/render_mermaid.hpp>
#include <exception>
#include <expressions/expression_parser.hpp>
#include <expressions/expression_policy.hpp>
#include <expressions/expression_read_only_dag_view.hpp>
#include <iostream>
#include <string>
#include <string_view>

int main(int argc, char** argv) {
  using namespace dagir::utility;

  if (argc < 2 || argc > 3) {
    std::cerr << "Usage: " << argv[0] << " <expression_file> [backend]\n"
              << "Supported backends: dot, json, mermaid (default: dot)\n";
    return 1;
  }

  const std::string filename = argv[1];
  const std::string backend = (argc == 3) ? std::string(argv[2]) : std::string("dot");

  try {
    // Read and parse the expression from the specified file
    my_expression_ptr expr = read_expression_from_file(filename);

    // Create a read-only DAG view over the parsed expression AST
    expression_read_only_dag_view dag_view(expr.get());

    // Build an intermediate representation (ir_graph) from the DAG view
    // Use expression-specific policies for node labels and edge attributes
    dagir::ir_graph ir = dagir::build_ir(dag_view, dagir::utility::expression_node_attributor{},
                                         dagir::utility::expression_edge_attributor{});

    // Render using the requested backend to stdout.
    std::ostream& os = std::cout;
    if (backend == "dot") {
      dagir::render_dot(os, ir, "expression");
    } else if (backend == "json") {
      dagir::render_json(os, ir);
    } else if (backend == "mermaid") {
      // Wrap the mermaid rendering call in a md file.
      os << "```mermaid\n";
      dagir::render_mermaid(os, ir, "expression");
      os << "```\n";
    } else {
      std::cerr << "Unknown backend: " << backend << "\n";
      std::cerr << "Supported backends: dot, json, mermaid\n";
      return 1;
    }

  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }

  return 0;
}