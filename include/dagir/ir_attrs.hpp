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

inline constexpr std::string_view kLabel{"label"};
inline constexpr std::string_view kTooltip{"tooltip"};
inline constexpr std::string_view kColor{"color"};
inline constexpr std::string_view kFillColor{"fillcolor"};
inline constexpr std::string_view kStyle{"style"};
inline constexpr std::string_view kShape{"shape"};
inline constexpr std::string_view kPenWidth{"penwidth"};
inline constexpr std::string_view kFontName{"fontname"};
inline constexpr std::string_view kFontSize{"fontsize"};
inline constexpr std::string_view kWeight{"weight"};
inline constexpr std::string_view kDir{"dir"};
inline constexpr std::string_view kId{"id"};
inline constexpr std::string_view kWidth{"width"};
inline constexpr std::string_view kHeight{"height"};
inline constexpr std::string_view kGroup{"group"};

// Graph-level keys
inline constexpr std::string_view kGraphLabel{"graph.label"};

}  // namespace ir_attrs
}  // namespace dagir
