/**
 * @file ir_serialization.hpp
 * @brief (De)serialization for `dagir::ir_graph` to/from JSON.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

#include "dagir/ir.hpp"
#include "dagir/ir_attrs.hpp"

// The implementation uses nlohmann::json when available. Consumers that
// wish to avoid an external dependency may provide their own implementation
// or build this translation unit with the header-only library available.
#if __has_include(<nlohmann/json.hpp>)
#include <nlohmann/json.hpp>
namespace dagir::serialize {

using json = nlohmann::json;

inline std::string to_json(const dagir::ir_graph& g) {
  json out;
  out["schema_version"] = 1;

  // nodes
  out["nodes"] = json::array();
  for (const auto& n : g.nodes) {
    json nn;
    // Prefer name attribute as id
    auto it_name = n.attributes.find(dagir::ir_attrs::k_name);
    if (it_name != n.attributes.end()) {
      nn["id"] = std::string(it_name->second);
    } else {
      nn["id"] = std::to_string(n.id);
    }
    if (n.attributes.count(dagir::ir_attrs::k_label))
      nn["label"] = std::string(n.attributes.at(dagir::ir_attrs::k_label));

    if (!n.attributes.empty()) {
      json attrs = json::object();
      for (const auto& p : n.attributes) {
        attrs[std::string(p.first)] = std::string(p.second);
      }
      nn["attributes"] = std::move(attrs);
    }
    out["nodes"].push_back(std::move(nn));
  }

  // edges
  out["edges"] = json::array();
  // For mapping ids to printable names, build a small lookup
  std::unordered_map<uint64_t, std::string> id_to_name;
  for (const auto& n : g.nodes) {
    auto it_name = n.attributes.find(dagir::ir_attrs::k_name);
    if (it_name != n.attributes.end())
      id_to_name[n.id] = std::string(it_name->second);
    else
      id_to_name[n.id] = std::to_string(n.id);
  }

  for (const auto& e : g.edges) {
    json ee;
    ee["source"] = id_to_name[e.source];
    ee["target"] = id_to_name[e.target];
    if (!e.attributes.empty()) {
      json attrs = json::object();
      for (const auto& p : e.attributes) attrs[std::string(p.first)] = std::string(p.second);
      ee["attributes"] = std::move(attrs);
    }
    out["edges"].push_back(std::move(ee));
  }

  if (!g.global_attrs.empty()) {
    json ga = json::object();
    for (const auto& p : g.global_attrs) ga[std::string(p.first)] = std::string(p.second);
    out["graphAttributes"] = std::move(ga);
  }

  return out.dump();
}

inline dagir::ir_graph from_json(std::string_view sv) {
  json in = json::parse(std::string(sv));
  if (!in.contains("schema_version")) throw std::invalid_argument("missing schema_version");
  int ver = in.at("schema_version").get<int>();
  if (ver != 1) throw std::invalid_argument("unsupported schema_version");

  if (!in.contains("nodes") || !in.contains("edges"))
    throw std::invalid_argument("missing required keys: nodes and/or edges");

  dagir::ir_graph g;

  // Map from JSON id string to numeric id
  std::unordered_map<std::string, uint64_t> id_map;
  uint64_t next_id = 1;

  // Read nodes
  for (const auto& nn : in.at("nodes")) {
    if (!nn.contains("id")) throw std::invalid_argument("node missing id");
    std::string jid = nn.at("id").get<std::string>();

    uint64_t numeric_id = 0;
    // Try parse as integer
    try {
      size_t idx = 0;
      unsigned long long v = std::stoull(jid, &idx);
      if (idx == jid.size()) numeric_id = static_cast<uint64_t>(v);
    } catch (...) {
      numeric_id = 0;
    }

    if (numeric_id == 0) numeric_id = next_id++;

    id_map[jid] = numeric_id;

    dagir::ir_node rn;
    rn.id = numeric_id;

    // Label
    if (nn.contains("label")) {
      auto sv_label = g.attr_cache.cache_view(nn.at("label").get<std::string>());
      rn.attributes[g.attr_cache.cache_view(dagir::ir_attrs::k_label)] = sv_label;
    }

    // Attributes
    if (nn.contains("attributes")) {
      for (auto it = nn.at("attributes").begin(); it != nn.at("attributes").end(); ++it) {
        auto key = g.attr_cache.cache_view(it.key());
        std::string sval =
            it.value().is_string() ? it.value().get<std::string>() : it.value().dump();
        auto val = g.attr_cache.cache_view(sval);
        rn.attributes[key] = val;
      }
    }

    // Preserve original id string when it isn't a plain numeric matching numeric_id
    if (jid != std::to_string(rn.id)) {
      rn.attributes[g.attr_cache.cache_view(dagir::ir_attrs::k_id)] = g.attr_cache.cache_view(jid);
    }

    g.nodes.push_back(std::move(rn));
  }

  // Build a map from numeric id to node index for later
  std::unordered_map<uint64_t, size_t> numeric_to_index;
  for (size_t i = 0; i < g.nodes.size(); ++i) numeric_to_index[g.nodes[i].id] = i;

  // Read edges
  for (const auto& ee : in.at("edges")) {
    if (!ee.contains("source") || !ee.contains("target"))
      throw std::invalid_argument("edge missing source/target");
    std::string ssrc = ee.at("source").get<std::string>();
    std::string stgt = ee.at("target").get<std::string>();
    if (id_map.find(ssrc) == id_map.end() || id_map.find(stgt) == id_map.end())
      throw std::invalid_argument("edge references unknown node id");

    dagir::ir_edge re;
    re.source = id_map[ssrc];
    re.target = id_map[stgt];

    if (ee.contains("attributes")) {
      for (auto it = ee.at("attributes").begin(); it != ee.at("attributes").end(); ++it) {
        auto key = g.attr_cache.cache_view(it.key());
        std::string sval =
            it.value().is_string() ? it.value().get<std::string>() : it.value().dump();
        auto val = g.attr_cache.cache_view(sval);
        re.attributes[key] = val;
      }
    }
    g.edges.push_back(std::move(re));
  }

  // global attrs
  if (in.contains("graphAttributes")) {
    for (auto it = in.at("graphAttributes").begin(); it != in.at("graphAttributes").end(); ++it) {
      auto key = g.attr_cache.cache_view(it.key());
      std::string sval = it.value().is_string() ? it.value().get<std::string>() : it.value().dump();
      auto val = g.attr_cache.cache_view(sval);
      g.global_attrs[key] = val;
    }
  }

  return g;
}

}  // namespace dagir::serialize
#else
namespace dagir::serialize {

inline std::string to_json(const dagir::ir_graph&) {
  throw std::runtime_error("ir_serialization requires nlohmann/json.hpp");
}

inline dagir::ir_graph from_json(std::string_view) {
  throw std::runtime_error("ir_serialization requires nlohmann/json.hpp");
}

}  // namespace dagir::serialize
#endif
