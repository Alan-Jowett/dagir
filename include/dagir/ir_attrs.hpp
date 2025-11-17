// SPDX-License-Identifier: MIT
// Copyright (c) DagIR Contributors
//
// Common IR attribute key names exposed as inline constexpr
// string_views so policy objects and backends can agree on
// the attribute keys they emit/consume without duplicating
// string literals throughout the codebase.

#pragma once

#include <string_view>

namespace dagir {
namespace ir_attrs {

/**
 * Common, renderer-neutral attribute keys.
 *
 * Notes:
 * - These keys are intentionally generic. Backends (JSON, Mermaid,
 *   GraphViz, etc.) should map these keys to renderer-specific
 *   names/semantics as required.
 * - Policy objects that produce `dagir::ir_attr` should use these
 *   constants to avoid typos and make intent explicit.
 * - We prefer `std::string_view` here to avoid static allocation
 *   order concerns and to keep header-light.
 */

inline constexpr std::string_view k_label{"label"};
inline constexpr std::string_view k_tooltip{"tooltip"};
inline constexpr std::string_view k_color{"color"};
inline constexpr std::string_view k_fill_color{"fillcolor"};
inline constexpr std::string_view k_style{"style"};
inline constexpr std::string_view k_shape{"shape"};
inline constexpr std::string_view k_pen_width{"penwidth"};
inline constexpr std::string_view k_font_name{"fontname"};
inline constexpr std::string_view k_font_size{"fontsize"};
inline constexpr std::string_view k_weight{"weight"};
inline constexpr std::string_view k_dir{"dir"};
inline constexpr std::string_view k_id{"id"};
inline constexpr std::string_view k_width{"width"};
inline constexpr std::string_view k_height{"height"};
inline constexpr std::string_view k_group{"group"};

// Graph-level keys
inline constexpr std::string_view k_graph_label{"graph.label"};

}  // namespace ir_attrs
}  // namespace dagir
