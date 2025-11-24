/**
 * @file tests/test_render_expr.cpp
 * @brief Unit tests for expression renderer
 */

#include <catch2/catch_test_macros.hpp>
#include <dagir/ir.hpp>
#include <dagir/render_expr.hpp>
#include <sstream>

TEST_CASE("render_expr emits terminals and simple variables", "[render_expr]") {
  dagir::ir_graph g;

  // Terminal 1
  dagir::ir_node t1;
  t1.id = 1;
  t1.attributes.emplace(dagir::ir_attrs::k_label, "1");
  /* SPDX-License-Identifier: MIT */
  /**
   * @file tests/test_render_expr.cpp
   * @brief Unit tests for expression renderer
   */

#include <catch2/catch_test_macros.hpp>
#include <dagir/ir.hpp>
#include <dagir/render_expr.hpp>
#include <sstream>

  TEST_CASE("render_expr emits terminals and simple variables", "[render_expr]") {
    dagir::ir_graph g;

    // Terminal 1
    dagir::ir_node t1;
    t1.id = 1;
    t1.attributes.emplace(dagir::ir_attrs::k_label, "1");
    g.nodes.push_back(t1);

    std::ostringstream os1;
    dagir::render_expr(os1, g);
    REQUIRE(os1.str().find("1") != std::string::npos);

    // Variable p: node 2 with label 'p', edges to 1 (true) and 3 (false)
    dagir::ir_node varp;
    varp.id = 2;
    varp.attributes.emplace(dagir::ir_attrs::k_label, "p");
    dagir::ir_node t0;
    t0.id = 3;
    t0.attributes.emplace(dagir::ir_attrs::k_label, "0");
    g.nodes.push_back(varp);
    g.nodes.push_back(t0);

    dagir::ir_edge e_true;
    e_true.source = 2;
    e_true.target = 1;
    e_true.attributes.emplace(dagir::ir_attrs::k_style, "solid");
    dagir::ir_edge e_false;
    e_false.source = 2;
    e_false.target = 3;
    e_false.attributes.emplace(dagir::ir_attrs::k_style, "dashed");
    g.edges.push_back(e_true);
    g.edges.push_back(e_false);

    std::ostringstream os2;
    dagir::render_expr(os2, g);
    auto out = os2.str();
    // Should emit at least the variable name for a canonical var-only BDD
    REQUIRE(out.find("p") != std::string::npos);
  }

  TEST_CASE("render_expr emits NOT and ITE forms", "[render_expr]") {
    dagir::ir_graph g;

    // Nodes: 10: constant 1, 11: constant 0
    dagir::ir_node c1{10, {}};
    c1.attributes.emplace(dagir::ir_attrs::k_label, "1");
    dagir::ir_node c0{11, {}};
    c0.attributes.emplace(dagir::ir_attrs::k_label, "0");
    // Node p -> true=c0 false=c1 gives NOT p
    dagir::ir_node p;
    p.id = 20;
    p.attributes.emplace(dagir::ir_attrs::k_label, "p");
    // Node q -> true=c1 false=c0 gives q
    dagir::ir_node q;
    q.id = 21;
    q.attributes.emplace(dagir::ir_attrs::k_label, "q");

    g.nodes = {c1, c0, p, q};

    dagir::ir_edge p_t;
    p_t.source = 20;
    p_t.target = 11;
    p_t.attributes.emplace(dagir::ir_attrs::k_style, "solid");
    dagir::ir_edge p_f;
    p_f.source = 20;
    p_f.target = 10;
    p_f.attributes.emplace(dagir::ir_attrs::k_style, "dashed");
    g.edges.push_back(p_t);
    g.edges.push_back(p_f);

    dagir::ir_edge q_t;
    q_t.source = 21;
    q_t.target = 10;
    q_t.attributes.emplace(dagir::ir_attrs::k_style, "solid");
    dagir::ir_edge q_f;
    q_f.source = 21;
    q_f.target = 11;
    q_f.attributes.emplace(dagir::ir_attrs::k_style, "dashed");
    g.edges.push_back(q_t);
    g.edges.push_back(q_f);

    std::ostringstream os;
    dagir::render_expr(os, g);
    std::string s = os.str();

    // Expect to see 'NOT p' for the p node contribution and 'q' for the q node
    REQUIRE(s.find("NOT p") != std::string::npos);
    /* SPDX-License-Identifier: MIT */
    /**
     * @file tests/test_render_expr.cpp
     * @brief Unit tests for expression renderer
     */

#include <catch2/catch_test_macros.hpp>
#include <dagir/ir.hpp>
#include <dagir/render_expr.hpp>
#include <sstream>

    TEST_CASE("render_expr emits terminals and simple variables", "[render_expr]") {
      dagir::ir_graph g;

      // Terminal 1
      dagir::ir_node t1;
      t1.id = 1;
      t1.attributes.emplace(dagir::ir_attrs::k_label, "1");
      g.nodes.push_back(t1);

      std::ostringstream os1;
      dagir::render_expr(os1, g);
      REQUIRE(os1.str().find("1") != std::string::npos);

      // Variable p: node 2 with label 'p', edges to 1 (true) and 3 (false)
      dagir::ir_node varp;
      varp.id = 2;
      varp.attributes.emplace(dagir::ir_attrs::k_label, "p");
      dagir::ir_node t0;
      t0.id = 3;
      t0.attributes.emplace(dagir::ir_attrs::k_label, "0");
      g.nodes.push_back(varp);
      g.nodes.push_back(t0);

      dagir::ir_edge e_true;
      e_true.source = 2;
      e_true.target = 1;
      e_true.attributes.emplace(dagir::ir_attrs::k_style, "solid");
      dagir::ir_edge e_false;
      e_false.source = 2;
      e_false.target = 3;
      e_false.attributes.emplace(dagir::ir_attrs::k_style, "dashed");
      g.edges.push_back(e_true);
      g.edges.push_back(e_false);

      std::ostringstream os2;
      dagir::render_expr(os2, g);
      auto out = os2.str();
      // Should emit at least the variable name for a canonical var-only BDD
      REQUIRE(out.find("p") != std::string::npos);
    }

    TEST_CASE("render_expr emits NOT and ITE forms", "[render_expr]") {
      dagir::ir_graph g;

      // Nodes: 10: constant 1, 11: constant 0
      dagir::ir_node c1{10, {}};
      c1.attributes.emplace(dagir::ir_attrs::k_label, "1");
      dagir::ir_node c0{11, {}};
      c0.attributes.emplace(dagir::ir_attrs::k_label, "0");
      // Node p -> true=c0 false=c1 gives NOT p
      dagir::ir_node p;
      p.id = 20;
      p.attributes.emplace(dagir::ir_attrs::k_label, "p");
      // Node q -> true=c1 false=c0 gives q
      dagir::ir_node q;
      q.id = 21;
      q.attributes.emplace(dagir::ir_attrs::k_label, "q");

      g.nodes = {c1, c0, p, q};

      dagir::ir_edge p_t;
      p_t.source = 20;
      p_t.target = 11;
      p_t.attributes.emplace(dagir::ir_attrs::k_style, "solid");
      dagir::ir_edge p_f;
      p_f.source = 20;
      p_f.target = 10;
      p_f.attributes.emplace(dagir::ir_attrs::k_style, "dashed");
      g.edges.push_back(p_t);
      g.edges.push_back(p_f);

      dagir::ir_edge q_t;
      q_t.source = 21;
      q_t.target = 10;
      q_t.attributes.emplace(dagir::ir_attrs::k_style, "solid");
      dagir::ir_edge q_f;
      q_f.source = 21;
      q_f.target = 11;
      q_f.attributes.emplace(dagir::ir_attrs::k_style, "dashed");
      g.edges.push_back(q_t);
      g.edges.push_back(q_f);

      std::ostringstream os;
      dagir::render_expr(os, g);
      std::string s = os.str();

      // Expect to see 'NOT p' for the p node contribution and 'q' for the q node
      REQUIRE(s.find("NOT p") != std::string::npos);
      REQUIRE(s.find("q") != std::string::npos);
    }
