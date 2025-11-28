#if !defined(SPDX_FILECOPYRIGHTTEXT)
// SPDX-FileCopyrightText: DagIR Contributors
// SPDX-License-Identifier: MIT
#endif
#include <catch2/catch_test_macros.hpp>
#include <dagir/render_dot.hpp>
#include <dagir/render_mermaid.hpp>
#include <dagir/themes.hpp>
#include <sstream>

TEST_CASE("dot_theme presets emit expected graph attributes", "[themes][dot]") {
  dagir::ir_graph g;
  std::ostringstream os;

  auto opts = dagir::dot_theme::dark();
  dagir::render_dot(os, g, "Test", std::cref(opts));
  const auto s = os.str();
  REQUIRE(s.find("bgcolor=\"#202124\"") != std::string::npos);
}

TEST_CASE("mermaid_theme presets emit init and classDefs", "[themes][mermaid]") {
  dagir::ir_graph g;
  std::ostringstream os;
  auto opts = dagir::mermaid_theme::diagnostics();
  dagir::render_mermaid(os, g, "Test", std::cref(opts));
  const auto s = os.str();
  REQUIRE(s.find("init: {\"theme\": \"default\"}") != std::string::npos);
  REQUIRE(s.find("classDef error") != std::string::npos);
}

TEST_CASE("dot_theme light and data_flow presets", "[themes][dot]") {
  dagir::ir_graph g;
  std::ostringstream os;
  auto light = dagir::dot_theme::light();
  dagir::render_dot(os, g, "Test", std::cref(light));
  REQUIRE(os.str().find("bgcolor=\"white\"") != std::string::npos);

  os.str(std::string());
  os.clear();
  auto df = dagir::dot_theme::data_flow();
  dagir::render_dot(os, g, "Test", std::cref(df));
  REQUIRE(os.str().find("penwidth=\"1.5\"") != std::string::npos);
}
