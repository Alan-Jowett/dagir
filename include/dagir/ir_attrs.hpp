// SPDX-License-Identifier: MIT
// Copyright (c) DagIR Contributors
//
// Canonical IR attribute key names exposed as `inline constexpr` string_view
// values. These keys are intended to be backend-neutral: backends should map
// them to renderer-specific attributes or fields when emitting output.

#pragma once

#include <string_view>

namespace dagir {
namespace ir_attrs {

/**
 * @file
 * @brief Canonical IR attribute keys and backend interpretation guidance.
 *
 * The constants in this header are the canonical keys used by policy objects
 * and IR builders. Backends are expected to interpret these keys and map
 * them into renderer-specific attribute names and semantics when producing
 * output (GraphViz, JSON, Mermaid, etc.). The documentation below explains
 * the intended meaning for each key and recommended backend behaviour.
 */

/**
 * @brief Primary label for nodes or edges.
 *
 * Intended semantics: a short, human-readable label representing the node
 * or edge. Backends should use this for the visible text label. For GraphViz
 * this maps to `label` (quoted string); for JSON backends it may be emitted
 * as the `label` property. If absent, backends may fall back to an id-based
 * label or an empty label.
 */
inline constexpr std::string_view k_label{"label"};

/**
 * @brief Tooltip / hover text.
 *
 * Intended semantics: auxiliary descriptive text presented on hover. Graphical
 * renderers should attach this to tooltip mechanisms (GraphViz `tooltip`,
 * HTML title attributes, or JS tooltip libraries). Non-interactive renderers
 * may choose to ignore this.
 */
inline constexpr std::string_view k_tooltip{"tooltip"};

/**
 * @brief Stroke/outline color for a shape or edge.
 *
 * Interpretation: backend-specific color string (named color, `#RRGGBB`,
 * or rgba). Map to GraphViz `color` for nodes/edges; map to CSS `stroke` or
 * `color` in HTML/SVG outputs. Backends should accept common color formats.
 */
inline constexpr std::string_view k_color{"color"};

/**
 * @brief Fill color for node shapes.
 *
 * Interpretation: backend-specific color string for node interior. Map to
 * GraphViz `fillcolor` and set `style=filled` where appropriate; for SVG/HTML
 * map to `fill` CSS property.
 */
inline constexpr std::string_view k_fill_color{"fillcolor"};

/**
 * @brief Visual style hints.
 *
 * Interpretation: free-form style string or a comma-separated list of style
 * tokens (e.g., `filled`, `dashed`, `bold`). For GraphViz, these can be
 * forwarded to the `style` attribute. Backends may define their own set of
 * supported tokens; unknown tokens should be ignored gracefully.
 */
inline constexpr std::string_view k_style{"style"};

/**
 * @brief Shape hint for node rendering.
 *
 * Interpretation: symbolic shape name (for example `box`, `ellipse`,
 * `diamond`). Backends should map to the nearest equivalent in their
 * renderer (GraphViz supports many shapes directly; HTML/SVG renderers may
 * map shapes to SVG primitives or CSS classes).
 */
inline constexpr std::string_view k_shape{"shape"};

/**
 * @brief Stroke thickness or pen width.
 *
 * Interpretation: numeric or unitless value indicating the thickness of the
 * outline. Backends should map to GraphViz `penwidth` or to appropriate CSS
 * properties (stroke-width) in SVG outputs.
 */
inline constexpr std::string_view k_pen_width{"penwidth"};

/**
 * @brief Preferred font family for text.
 *
 * Interpretation: a font family name (e.g., `Arial`, `Monospace`). Backends
 * should attempt to apply the font where supported; if not available they may
 * fall back to a default font.
 */
inline constexpr std::string_view k_font_name{"fontname"};

/**
 * @brief Preferred font size for text.
 *
 * Interpretation: numeric or unit string indicating desired font size. Map to
 * GraphViz `fontsize` or CSS `font-size` in HTML/SVG renderers.
 */
inline constexpr std::string_view k_font_size{"fontsize"};

/**
 * @brief Rendering weight or importance hint.
 *
 * Interpretation: free-form numeric or string value indicating emphasis or
 * weighting for layout engines. For layout backends that support weighting
 * (e.g., GraphViz `weight` on edges), this value may influence edge routing
 * or ordering. Backends that don't support it should ignore it.
 */
inline constexpr std::string_view k_weight{"weight"};

/**
 * @brief Edge direction hint.
 *
 * Interpretation: indicates the directionality of the edge. Typical values
 * include `forward`, `back`, `both`, or `none`. Map to GraphViz `dir` where
 * appropriate (`forward`, `back`, `both`, `none`). If not provided assume
 * default directed behaviour or consult graph-level semantics.
 */
inline constexpr std::string_view k_dir{"dir"};
/**
 * @brief Renderer rank direction hint (e.g. `TB`, `LR`).
 *
 * Interpretation: when present at graph scope this key specifies the preferred
 * layout direction. Backends that support a `rankdir` or equivalent setting
 * should map this value directly. If absent, renderers may choose a
 * sensible default (GraphViz commonly defaults to `TB`).
 */
inline constexpr std::string_view k_rankdir{"rankdir"};

/**
 * @brief Explicit identifier for a graph element.
 *
 * Interpretation: an id string that backends should use as the stable object
 * identifier when possible. For GraphViz this may map to the node name
 * (unquoted identifier) or to an `id` attribute in JSON outputs. Backends
 * must ensure identifiers are escaped/normalized where required by the
 * renderer.
 */
inline constexpr std::string_view k_id{"id"};

/**
 * @brief Preferred width for node rendering.
 *
 * Interpretation: numeric value in renderer-specific units (for GraphViz
 * typically inches) indicating the desired node width. Backends may treat
 * this as a suggestion rather than an absolute size.
 */
inline constexpr std::string_view k_width{"width"};

/**
 * @brief Preferred height for node rendering.
 *
 * Interpretation: numeric value in renderer-specific units indicating the
 * desired node height. Backends may treat this as a suggestion.
 */
inline constexpr std::string_view k_height{"height"};

/**
 * @brief Shortest-hop distance from graph roots.
 *
 * Interpretation: an integer encoded as a string representing the minimal
 * number of edges from any root to this node. Backends may use this for
 * level-based layout, coloring, or filtering. Unreachable nodes may be
 * represented with `-1`.
 */
inline constexpr std::string_view k_rank{"rank"};

/**
 * @brief Grouping key for layout engines.
 *
 * Interpretation: arbitrary string used to group nodes together for layout
 * or styling purposes. For GraphViz this may map to `group`; for other
 * backends it may be used to assign nodes to clusters or CSS classes.
 */
inline constexpr std::string_view k_group{"group"};

// Graph-level keys
/**
 * @brief Graph-level human-readable label.
 *
 * Interpretation: a caption or title for the whole graph. Backends should
 * emit this as a top-level label/legend where supported (GraphViz `label`
 * on the graph object, HTML headings, or JSON `label` field).
 */
inline constexpr std::string_view k_graph_label{"graph.label"};

}  // namespace ir_attrs
}  // namespace dagir
