/*
 * @file
 * @brief Theme preset helpers for DOT and Mermaid renderers.
 *
 * Small header-only helpers exposing option structs and a few ready-made
 * presets (light, dark, diagnostics, data-flow) that callers can layer
 * with custom overrides.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace dagir {

struct dot_options {
  // Graph-level defaults (if the graph doesn't already provide them).
  std::optional<std::string> bgcolor;
  std::optional<std::string> rankdir;

  // Node defaults applied when a node doesn't provide the attribute.
  std::optional<std::string> node_shape;
  std::optional<std::string> node_style;
  std::optional<std::string> node_fill_color;
  std::optional<std::string> node_fontcolor;

  // Edge defaults
  std::optional<std::string> edge_color;
  std::optional<std::string> edge_penwidth;

  // Misc
  std::optional<std::string> fontname;
  std::optional<std::string> fontsize;
};

struct mermaid_options {
  // Mermaid theme name used in init directive (e.g. "default", "dark").
  std::string mermaid_theme = "default";

  // Optional rankdir default if graph doesn't specify one.
  std::optional<std::string> rankdir;

  // Arbitrary classDef block(s) injected before the graph to supply styles.
  // Each string should be a complete `classDef ...` line (without trailing newline).
  std::vector<std::string> class_defs;

  // Map of node id -> class name to apply via `class <id> <classname>` lines.
  std::unordered_map<std::string, std::string> node_classes;
};

namespace dot_theme {
inline dot_options light() {
  dot_options o;
  o.bgcolor = "white";
  o.node_shape = "box";
  o.node_style = "filled";
  o.node_fill_color = "#ffffff";
  o.node_fontcolor = "#000000";
  o.edge_color = "#000000";
  o.fontname = "Helvetica";
  o.fontsize = "10";
  return o;
}

inline dot_options dark() {
  dot_options o;
  o.bgcolor = "#202124";
  o.node_shape = "box";
  o.node_style = "filled";
  o.node_fill_color = "#2e2e2e";
  o.node_fontcolor = "#e8eaed";
  o.edge_color = "#9aa0a6";
  o.fontname = "Helvetica";
  o.fontsize = "10";
  return o;
}

inline dot_options diagnostics() {
  dot_options o = light();
  // Emphasize errors/critical paths with red edges/nodes by default
  o.edge_color = "#d93025";       // red
  o.node_fill_color = "#fff5f5";  // subtle red tint
  return o;
}

inline dot_options data_flow() {
  dot_options o = light();
  o.edge_color = "#1a73e8";  // blue
  o.edge_penwidth = "1.5";
  return o;
}
}  // namespace dot_theme

namespace mermaid_theme {
inline mermaid_options light() {
  mermaid_options o;
  o.mermaid_theme = "default";
  return o;
}

inline mermaid_options dark() {
  mermaid_options o;
  o.mermaid_theme = "dark";
  return o;
}

inline mermaid_options diagnostics() {
  mermaid_options o;
  o.mermaid_theme = "default";
  // simple classDef highlighting for error nodes
  o.class_defs.push_back("classDef error fill:#ffdce0,stroke:#d93025,stroke-width:2px");
  o.class_defs.push_back("classDef critical fill:#fff4e5,stroke:#f29900,stroke-width:2px");
  // users can assign classes via node_classes map
  return o;
}

inline mermaid_options data_flow() {
  mermaid_options o;
  o.mermaid_theme = "default";
  o.class_defs.push_back("classDef flowEdge stroke:#1a73e8,stroke-width:2px");
  return o;
}
}  // namespace mermaid_theme

}  // namespace dagir
