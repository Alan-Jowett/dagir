/**
 * @file werror_compile_check.cpp
 * @brief Test compilation unit to enforce warnings-as-errors on public headers.
 *
 * @details
 * This compilation unit includes all public DagIR headers to ensure that
 * they compile cleanly with warnings treated as errors. This helps maintain
 * high code quality and prevents the introduction of new warnings.
 *
 * @copyright
 * © DagIR Contributors. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

// Minimal compilation unit to enforce warnings-as-errors on public headers
#include <dagir/algorithms.hpp>
#include <dagir/build_ir.hpp>
#include <dagir/ir.hpp>
#include <dagir/ir_attrs.hpp>
#include <dagir/node_id.hpp>
#include <dagir/render_dot.hpp>
#include <dagir/render_json.hpp>
#include <dagir/render_mermaid.hpp>
#include <dagir/render_svg.hpp>
#include <dagir/sugiyama.hpp>

int main() {
  return 0;
}
