/**
 * @file
 * @brief Header-only JSON renderer for `dagir::ir_graph` matching
 * `docs/dagir_json_schema.json`.
 *
 * This header emits a JSON object matching the schema in
 * `docs/dagir_json_schema.json`. The top-level object contains the
 * required properties `nodes` and `edges` and the optional properties
 * `roots` and `graphAttributes` when present. Node and edge attributes are
 * emitted as JSON objects in the schema's `attributes` form; values that
 * parse as JSON primitives (number, boolean, null) are emitted as primitives
 * to preserve type when round-tripping.
 *
 * The implementation is intentionally header-only and conservative about
 * escaping so it can be used in tests and small tools without extra
 * dependencies.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <algorithm>
#include <cerrno>
#include <charconv>
#include <cstdlib>
#include <dagir/ir.hpp>
#include <dagir/ir_attrs.hpp>
#include <iomanip>
#include <map>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>

namespace dagir {

namespace render_json_detail {

/**
 * @brief Escape a string for inclusion in JSON string literals.
 *
 * Produces a JSON-safe string by escaping quotes, backslashes and control
 * characters. The result is suitable to be written inside double quotes.
 */
inline std::string escape_json_string(const std::string& s) {
  std::ostringstream o;
  for (unsigned char c : s) {
    switch (c) {
      case '"':
        o << "\\\"";
        break;
      case '\\':
        o << "\\\\";
        break;
      case '\b':
        o << "\\b";
        break;
      case '\f':
        o << "\\f";
        break;
      case '\n':
        o << "\\n";
        break;
      case '\r':
        o << "\\r";
        break;
      case '\t':
        o << "\\t";
        break;
      default:
        if (c < 0x20) {
          o << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c)
            << std::dec << std::setfill(' ');
        } else {
          o << static_cast<char>(c);
        }
    }
  }
  return o.str();
}

/**
 * @brief Attempt to interpret a string as a JSON primitive.
 *
 * If `s` is exactly `null`, `true`, `false`, an integer, or a floating
 * point literal, this returns a string containing the literal representation
 * (without surrounding quotes). Otherwise returns `std::nullopt` indicating
 * the value should be emitted as a JSON string.
 */
inline std::optional<std::string> try_emit_primitive(const std::string& s) {
  if (s == "null") return std::string("null");
  if (s == "true") return std::string("true");
  if (s == "false") return std::string("false");

  // Try integer using from_chars (no locale dependency)
  long long iv = 0;
  auto res = std::from_chars(s.data(), s.data() + s.size(), iv);
  if (res.ec == std::errc() && res.ptr == s.data() + s.size()) {
    return std::to_string(iv);
  }

  // Fallback: try floating point via strtod
  errno = 0;
  char* endptr = nullptr;
  double d = std::strtod(s.c_str(), &endptr);
  if (endptr == s.c_str() + s.size() && errno == 0) {
    std::ostringstream os;
    os << std::setprecision(15) << d;
    return os.str();
  }

  return std::nullopt;
}

/**
 * @brief Convert an attribute vector to a map for easier lookup.
 */
// Attributes are now stored as `ir_attr_map`; conversion helper removed.

}  // namespace render_json_detail

/**
 * @brief Render `ir_graph` as JSON to the provided output stream.
 *
 * The output conforms to `docs/dagir_json_schema.json` and includes the
 * `nodes` and `edges` arrays. Optional `roots` and `graphAttributes` are
 * emitted when present. Node and edge `attributes` are emitted as JSON
 * objects; values that can be parsed as numbers, booleans or `null` are
 * emitted as JSON primitives to preserve their type when possible.
 *
 * @param os Stream to write JSON to.
 * @param g The intermediate representation to serialize.
 */
inline void render_json(std::ostream& os, const ir_graph& g) {
  os << "{";

  // nodes
  os << "\"nodes\": [";
  bool first_node = true;
  for (const auto& n : g.nodes) {
    if (!first_node) os << ", ";
    first_node = false;
    os << "{";
    // Prefer attribute "name" as the node identifier; fall back to numeric id.
    const auto& amap = n.attributes;
    if (amap.count("name"))
      os << "\"id\": \"" << render_json_detail::escape_json_string(amap.at("name")) << "\"";
    else
      os << "\"id\": \"" << render_json_detail::escape_json_string(std::to_string(n.id)) << "\"";
    // Emit label from attributes if present
    if (amap.count(ir_attrs::k_label)) {
      os << ", \"label\": \"" << render_json_detail::escape_json_string(amap.at(ir_attrs::k_label))
         << "\"";
    }
    if (!n.attributes.empty()) {
      os << ", \"attributes\": {";
      bool first_attr = true;
      std::vector<std::string> keys;
      keys.reserve(n.attributes.size());
      std::transform(n.attributes.begin(), n.attributes.end(), std::back_inserter(keys),
                     [](auto const& p) { return std::string(p.first); });
      std::sort(keys.begin(), keys.end());
      for (const auto& k : keys) {
        if (!first_attr) os << ", ";
        first_attr = false;
        const auto& val = n.attributes.at(k);
        os << "\"" << render_json_detail::escape_json_string(k) << "\": ";
        if (auto prim = render_json_detail::try_emit_primitive(val)) {
          os << *prim;
        } else {
          os << "\"" << render_json_detail::escape_json_string(val) << "\"";
        }
      }
      os << "}";
    }
    os << "}";
  }
  os << "]";

  // edges
  os << ", \"edges\": [";
  bool first_edge = true;
  for (const auto& e : g.edges) {
    if (!first_edge) os << ", ";
    first_edge = false;
    os << "{";
    // For edges, attempt to use node `name` where available. Look up the
    // nodes by id in the graph to find their names; fall back to numeric id.
    auto find_node_name = [&](std::uint64_t nid) -> std::string {
      auto it = std::find_if(g.nodes.begin(), g.nodes.end(),
                             [&](const ir_node& nn) { return nn.id == nid; });
      if (it != g.nodes.end()) {
        const auto& aam = it->attributes;
        if (aam.count("name")) return aam.at("name");
        return std::to_string(it->id);
      }
      return std::to_string(nid);
    };

    os << "\"source\": \"" << render_json_detail::escape_json_string(find_node_name(e.source))
       << "\",";
    os << " \"target\": \"" << render_json_detail::escape_json_string(find_node_name(e.target))
       << "\"";
    if (!e.attributes.empty()) {
      os << ", \"attributes\": {";
      bool first_attr = true;
      std::vector<std::string> keys;
      keys.reserve(e.attributes.size());
      std::transform(e.attributes.begin(), e.attributes.end(), std::back_inserter(keys),
                     [](auto const& p) { return std::string(p.first); });
      std::sort(keys.begin(), keys.end());
      for (const auto& k : keys) {
        if (!first_attr) os << ", ";
        first_attr = false;
        const auto& val = e.attributes.at(k);
        os << "\"" << render_json_detail::escape_json_string(k) << "\": ";
        if (auto prim = render_json_detail::try_emit_primitive(val)) {
          os << *prim;
        } else {
          os << "\"" << render_json_detail::escape_json_string(val) << "\"";
        }
      }
      os << "}";
    }
    os << "}";
  }
  os << "]";

  // `ir_graph` does not currently contain roots; the JSON schema allows
  // an optional `roots` array, but the renderer only emits fields present
  // on the IR. If roots are added to the IR in future, emit them here.

  // optional graphAttributes - emit remaining global_attrs not handled as keys
  if (!g.global_attrs.empty()) {
    os << ", \"graphAttributes\": {";
    bool first_ga = true;
    std::vector<std::string> gkeys;
    gkeys.reserve(g.global_attrs.size());
    std::transform(g.global_attrs.begin(), g.global_attrs.end(), std::back_inserter(gkeys),
                   [](auto const& p) { return std::string(p.first); });
    std::sort(gkeys.begin(), gkeys.end());
    for (const auto& k : gkeys) {
      if (!first_ga) os << ", ";
      first_ga = false;
      const auto& v = g.global_attrs.at(k);
      os << "\"" << render_json_detail::escape_json_string(k) << "\": ";
      if (auto prim = render_json_detail::try_emit_primitive(v)) {
        os << *prim;
      } else {
        os << "\"" << render_json_detail::escape_json_string(v) << "\"";
      }
    }
    os << "}";
  }

  os << "}";
}

}  // namespace dagir
