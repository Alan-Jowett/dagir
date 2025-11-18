/**
 * @file tests/test_expression_parser.cpp
 * @brief Unit tests for expression_parser utilities
 * @copyright
 * © DagIR Contributors. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

#include <catch2/catch_test_macros.hpp>
#include <dagir/utility/expression_ast.hpp>
#include <dagir/utility/expression_parser.hpp>
#include <filesystem>

using namespace dagir::utility;

static std::string var_name_from_expr(const my_expression_ptr& p) {
  auto* v = std::get_if<my_variable>(p.get());
  if (!v) return std::string();
  return v->variable_name;
}

TEST_CASE("trim removes surrounding whitespace", "[expression_parser]") {
  REQUIRE(trim("  abc ") == "abc");
  REQUIRE(trim("\t\n x ") == "x");
}

TEST_CASE("Tokenizer recognizes simple tokens", "[expression_parser][tokenizer]") {
  Tokenizer tz("  x0 AND ( NOT y ) OR z XOR w ");
  auto t = tz.next_token();
  REQUIRE(t.type == Tokenizer::TokenType::VARIABLE);
  REQUIRE(t.value == "x0");

  t = tz.next_token();
  REQUIRE(t.type == Tokenizer::TokenType::AND);

  t = tz.next_token();
  REQUIRE(t.type == Tokenizer::TokenType::LPAREN);

  t = tz.next_token();
  REQUIRE(t.type == Tokenizer::TokenType::NOT);

  t = tz.next_token();
  REQUIRE(t.type == Tokenizer::TokenType::VARIABLE);
  REQUIRE(t.value == "y");

  // drain remaining tokens to EOF
  while ((t = tz.next_token()).type != Tokenizer::TokenType::EOF_TOKEN) {
  }
}

TEST_CASE("Parser parses variables and parentheses correctly", "[expression_parser][parser]") {
  auto expr = parse_expression("(a)");
  REQUIRE(expr);
  REQUIRE(std::holds_alternative<my_variable>(*expr));
  REQUIRE(var_name_from_expr(expr) == "a");
}

TEST_CASE("Operator precedence: NOT binds tightest, XOR lowest",
          "[expression_parser][precedence]") {
  // a AND b OR c  -> parsed as ( (a AND b) OR c )
  auto e1 = parse_expression("a AND b OR c");
  REQUIRE(e1);
  REQUIRE(std::holds_alternative<my_or>(*e1));

  // NOT NOT a -> double negation nests correctly
  auto e2 = parse_expression("NOT NOT a");
  REQUIRE(e2);
  REQUIRE(std::holds_alternative<my_not>(*e2));
  auto& n1 = std::get<my_not>(*e2);
  REQUIRE(std::holds_alternative<my_not>(*n1.expr));

  // XOR has lowest precedence: a XOR b OR c -> (a XOR (b OR c))
  auto e3 = parse_expression("a XOR b OR c");
  REQUIRE(e3);
  REQUIRE(std::holds_alternative<my_xor>(*e3));
}

TEST_CASE("Parse rejects empty strings", "[expression_parser][error]") {
  REQUIRE_THROWS_AS(parse_expression("   "), std::runtime_error);
}

TEST_CASE("read_expression_from_file errors on missing file", "[expression_parser][file]") {
  REQUIRE_THROWS_AS(read_expression_from_file("non_existent_file.expr"), std::runtime_error);
}

TEST_CASE("Parse all expression files in tests/expressions", "[expression_parser][files]") {
  namespace fs = std::filesystem;
  fs::path test_src = __FILE__;
  fs::path expr_dir = test_src.parent_path() / "expressions";

  REQUIRE(fs::exists(expr_dir));

  for (auto const& entry : fs::directory_iterator(expr_dir)) {
    if (!entry.is_regular_file()) continue;
    if (entry.path().extension() != ".expr") continue;

    INFO("Parsing expression file: " << entry.path().string());
    my_expression_ptr parsed;
    REQUIRE_NOTHROW(parsed = read_expression_from_file(entry.path().string()));
    REQUIRE(parsed != nullptr);
  }
}
