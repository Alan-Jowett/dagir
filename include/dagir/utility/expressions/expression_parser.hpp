/**
 * @file expression_parser.hpp
 * @brief Helper utility for parsing string binary expressions
 *
 * @details
 *
 * @copyright
 * © DagIR Contributors. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cctype>
#include <dagir/utility/expressions/expression_ast.hpp>
#include <format>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace dagir {
namespace utility {

/**
 * @brief Trims leading and trailing whitespace from a string
 */
inline std::string trim(const std::string& str) {
  size_t start = str.find_first_not_of(" \t\n\r");
  if (start == std::string::npos) return "";
  size_t end = str.find_last_not_of(" \t\n\r");
  return str.substr(start, end - start + 1);
}

/**
 * @brief Tokenizer for logical expressions
 */
class Tokenizer {
 private:
  std::string text;
  size_t pos = 0;

  /**
   * @brief Skips whitespace characters at current position
   *
   * Advances the position pointer past any whitespace characters.
   * Uses unsigned char casting to safely handle extended ASCII.
   */
  void skip_whitespace() {
    while (pos < text.length() && std::isspace(static_cast<unsigned char>(text[pos]))) {
      pos++;
    }
  }

 public:
  enum class TokenType { VARIABLE, AND, OR, XOR, NOT, LPAREN, RPAREN, EOF_TOKEN };

  struct Token {
    TokenType type;
    std::string value;
    size_t position;

    // C++20 spaceship operator for automatic comparison generation
    auto operator<=>(const Token& other) const = default;
  };

  /**
   * @brief Converts TokenType enum to human-readable string
   *
   * @param type The token type to convert
   * @return String representation of the token type for error messages
   */
  static std::string token_type_to_string(TokenType type) {
    switch (type) {
      case TokenType::VARIABLE:
        return "variable";
      case TokenType::AND:
        return "AND";
      case TokenType::OR:
        return "OR";
      case TokenType::XOR:
        return "XOR";
      case TokenType::NOT:
        return "NOT";
      case TokenType::LPAREN:
        return "'('";
      case TokenType::RPAREN:
        return "')'";
      case TokenType::EOF_TOKEN:
        return "end of input";
      default:
        return "unknown token";
    }
  }

  /**
   * @brief Constructs a tokenizer with input text
   *
   * @param input The expression string to tokenize
   */
  explicit Tokenizer(const std::string& input) : text(input) {}

  /**
   * @brief Gets the next token from the input stream
   *
   * Performs lexical analysis to identify operators, parentheses, and variables.
   * Handles proper boundary detection for operator keywords within variable names.
   *
   * @return The next token with type, value, and position information
   * @throws std::runtime_error If an unexpected character is encountered
   */
  Token next_token() {
    skip_whitespace();

    if (pos >= text.length()) {
      return {.type = TokenType::EOF_TOKEN, .value = "", .position = pos};
    }

    size_t start_pos = pos;

    // Check for operators and parentheses
    auto is_boundary_char = [&](size_t idx) {
      if (idx >= text.length()) {
        return true;
      }
      char ch = text[idx];
      unsigned char c = static_cast<unsigned char>(ch);
      return std::isspace(c) || ch == '(' || ch == ')';
    };
    auto has_boundary_before = [&](size_t idx) {
      if (idx == 0) {
        return true;
      }
      char ch = text[idx - 1];
      unsigned char c = static_cast<unsigned char>(ch);
      return std::isspace(c) || ch == '(' || ch == ')';
    };

    if (pos + 3 <= text.length() && text.compare(pos, 3, "AND") == 0 && has_boundary_before(pos) &&
        is_boundary_char(pos + 3)) {
      pos += 3;
      return {.type = TokenType::AND, .value = "AND", .position = start_pos};
    }
    if (pos + 2 <= text.length() && text.compare(pos, 2, "OR") == 0 && has_boundary_before(pos) &&
        is_boundary_char(pos + 2)) {
      pos += 2;
      return {.type = TokenType::OR, .value = "OR", .position = start_pos};
    }
    if (pos + 3 <= text.length() && text.compare(pos, 3, "XOR") == 0 && has_boundary_before(pos) &&
        is_boundary_char(pos + 3)) {
      pos += 3;
      return {.type = TokenType::XOR, .value = "XOR", .position = start_pos};
    }
    if (pos + 3 <= text.length() && text.compare(pos, 3, "NOT") == 0 && has_boundary_before(pos) &&
        is_boundary_char(pos + 3)) {
      pos += 3;
      return {.type = TokenType::NOT, .value = "NOT", .position = start_pos};
    }
    if (text[pos] == '(') {
      pos++;
      return {.type = TokenType::LPAREN, .value = "(", .position = start_pos};
    }
    if (text[pos] == ')') {
      pos++;
      return {.type = TokenType::RPAREN, .value = ")", .position = start_pos};
    }

    // Parse variable name (any non-whitespace that's not an operator or parentheses)
    if (!std::isspace(static_cast<unsigned char>(text[pos])) && text[pos] != '(' &&
        text[pos] != ')') {
      std::string var_name;
      while (pos < text.length() && !std::isspace(static_cast<unsigned char>(text[pos])) &&
             text[pos] != '(' && text[pos] != ')') {
        var_name += text[pos];
        pos++;
      }

      if (!var_name.empty()) {
        return {.type = TokenType::VARIABLE, .value = var_name, .position = start_pos};
      }
    }

    throw std::runtime_error(
        std::format("Unexpected character at position {}: '{}'", pos, text[pos]));
  }

  /**
   * @brief Peeks at the next token without advancing the position
   *
   * @return The next token that would be returned by next_token()
   */
  Token peek_token() {
    size_t saved_pos = pos;
    Token token = next_token();
    pos = saved_pos;
    return token;
  }
};

/**
 * @brief Recursive descent parser for logical expressions
 *
 * Grammar (in order of precedence, lowest to highest):
 * expression -> xor_expr
 * xor_expr -> or_expr (XOR or_expr)*
 * or_expr -> and_expr (OR and_expr)*
 * and_expr -> not_expr (AND not_expr)*
 * not_expr -> NOT not_expr | primary
 * primary -> VARIABLE | ( expression )
 */
class Parser {
 private:
  Tokenizer tokenizer;
  Tokenizer::Token current_token;

  /**
   * @brief Advances to the next token in the input stream
   *
   * Updates current_token by calling the tokenizer's next_token() method.
   */
  void advance() { current_token = tokenizer.next_token(); }

  /**
   * @brief Consumes current token if it matches expected type, otherwise throws error
   *
   * @param expected The token type that is expected
   * @throws std::runtime_error If current token doesn't match expected type
   */
  void expect(Tokenizer::TokenType expected) {
    if (current_token.type != expected) {
      throw std::runtime_error(std::format(
          "Expected {} but got {} at position {}", Tokenizer::token_type_to_string(expected),
          Tokenizer::token_type_to_string(current_token.type), current_token.position));
    }
    advance();
  }

  /**
   * @brief Parses a primary expression (variable or parenthesized expression)
   *
   * Primary expressions are the highest precedence level in the grammar.
   *
   * @return Pointer to the parsed expression
   * @throws std::runtime_error If neither a variable nor '(' is found
   */
  my_expression_ptr parse_primary() {
    if (current_token.type == Tokenizer::TokenType::VARIABLE) {
      std::string var_name = current_token.value;
      advance();
      return std::make_unique<my_expression>(my_variable{var_name});
    } else if (current_token.type == Tokenizer::TokenType::LPAREN) {
      advance();  // consume '('
      auto expr = parse_expression();
      expect(Tokenizer::TokenType::RPAREN);
      return expr;
    } else {
      throw std::runtime_error(
          std::format("Expected variable or '(' at position {}", current_token.position));
    }
  }

  /**
   * @brief Parses NOT expressions (unary, right-associative)
   *
   * Handles unary NOT operations with proper right-associative parsing.
   * Falls through to parse_primary() for non-NOT expressions.
   *
   * @return Pointer to the parsed NOT expression or primary expression
   */
  my_expression_ptr parse_not_expr() {
    if (current_token.type == Tokenizer::TokenType::NOT) {
      advance();                        // consume 'NOT'
      auto operand = parse_not_expr();  // right-associative
      return std::make_unique<my_expression>(my_not{std::move(operand)});
    } else {
      return parse_primary();
    }
  }

  /**
   * @brief Parses AND expressions with left-associative binary operations
   *
   * Handles binary AND operations with proper left-associative parsing.
   * Parses sequences like: expr AND expr AND expr
   *
   * @return Pointer to the parsed AND expression tree
   */
  my_expression_ptr parse_and_expr() {
    auto left = parse_not_expr();

    while (current_token.type == Tokenizer::TokenType::AND) {
      advance();  // consume 'AND'
      auto right = parse_not_expr();
      left = std::make_unique<my_expression>(my_and{std::move(left), std::move(right)});
    }

    return left;
  }

  /**
   * @brief Parses OR expressions with left-associative binary operations
   *
   * Handles binary OR operations with proper left-associative parsing.
   * Parses sequences like: expr OR expr OR expr
   *
   * @return Pointer to the parsed OR expression tree
   */
  my_expression_ptr parse_or_expr() {
    auto left = parse_and_expr();

    while (current_token.type == Tokenizer::TokenType::OR) {
      advance();  // consume 'OR'
      auto right = parse_and_expr();
      left = std::make_unique<my_expression>(my_or{std::move(left), std::move(right)});
    }

    return left;
  }

  /**
   * @brief Parses XOR expressions with left-associative binary operations
   *
   * Handles binary XOR operations with proper left-associative parsing.
   * Parses sequences like: expr XOR expr XOR expr. This is the top-level
   * expression parsing method due to XOR having the lowest precedence.
   *
   * @return Pointer to the parsed XOR expression tree
   */
  my_expression_ptr parse_xor_expr() {
    auto left = parse_or_expr();

    while (current_token.type == Tokenizer::TokenType::XOR) {
      advance();  // consume 'XOR'
      auto right = parse_or_expr();
      left = std::make_unique<my_expression>(my_xor{std::move(left), std::move(right)});
    }

    return left;
  }

 public:
  /**
   * @brief Constructs a parser with input text
   *
   * @param input The expression string to parse
   */
  explicit Parser(const std::string& input) : tokenizer(input) {
    advance();  // Initialize current_token
  }

  /**
   * @brief Parses the top-level expression (entry point for grammar)
   *
   * @return Pointer to the parsed expression tree
   */
  my_expression_ptr parse_expression() { return parse_xor_expr(); }

  /**
   * @brief Parses a complete expression and ensures no trailing tokens
   *
   * @return Pointer to the parsed expression tree
   * @throws std::runtime_error If unexpected tokens remain after parsing
   */
  my_expression_ptr parse() {
    auto expr = parse_expression();
    if (current_token.type != Tokenizer::TokenType::EOF_TOKEN) {
      throw std::runtime_error(
          std::format("Unexpected token after expression at position {}", current_token.position));
    }
    return expr;
  }
};

/**
 * @brief Parses a logical expression string into an expression tree
 *
 * Clean recursive descent parser that handles operator precedence correctly.
 * Supports proper operator precedence (highest to lowest):
 * 1. NOT (unary, right-associative)
 * 2. AND (binary, left-associative)
 * 3. OR (binary, left-associative)
 * 4. XOR (binary, left-associative)
 *
 * @param expr_str String representation of the logical expression
 * @return Smart pointer to the root of the parsed expression tree
 */
inline my_expression_ptr parse_expression(const std::string& expr_str) {
  std::string trimmed = trim(expr_str);

  if (trimmed.empty()) {
    throw std::runtime_error("Empty expression encountered during parsing");
  }

  try {
    Parser parser(trimmed);
    return parser.parse();
  } catch (const std::exception& e) {
    throw std::runtime_error("Parse error in expression '" + trimmed + "': " + e.what());
  }
}

/**
 * @brief Reads and parses a logical expression from a text file
 *
 * Reads all non-empty, non-comment lines from the specified file and
 * concatenates them into a single expression string. Lines starting with '#'
 * are treated as comments and ignored.
 *
 * @param filename Path to the file containing the expression
 * @return Smart pointer to the parsed expression tree
 *
 * @throws std::runtime_error If file cannot be opened or no valid expression is found
 *
 * File format:
 * - One or more lines containing parts of the expression
 * - Empty lines are ignored
 * - Lines starting with '#' are treated as comments
 * - Expression parts are concatenated with spaces
 *
 * Example file content:
 * ```
 * # This is a comment
 * (x0 AND x1) OR
 * (NOT x2) XOR (x3 AND (NOT x4))
 * ```
 */
inline my_expression_ptr read_expression_from_file(const std::string& filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    throw std::runtime_error("Could not open file: " + filename);
  }

  std::string line;
  std::string expression_str;

  // Read all non-empty lines and concatenate them
  while (std::getline(file, line)) {
    line = trim(line);
    if (!line.empty() && line[0] != '#') {  // Skip empty lines and comments
      expression_str += " " + line;
    }
  }

  expression_str = trim(expression_str);
  if (expression_str.empty()) {
    throw std::runtime_error("No expression found in file: " + filename);
  }

  return parse_expression(expression_str);
}

}  // namespace utility
}  // namespace dagir