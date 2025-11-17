SPDX-License-Identifier: MIT

## Implementing Policy Objects for DagIR

This document explains how to implement policy objects that produce node labels and edge attributes for the DagIR `ir_graph`. It also documents the common IR attribute names provided by `dagir::ir_attrs` and the intended usage for backends (JSON, Mermaid, GraphViz, etc.).

## Overview

DagIR separates traversal from rendering via small policy objects:

- `NodeLabeler` — callable used to produce a `std::string` label for a node.
- `EdgeAttributor` — callable used to produce attributes for an edge (return type interpreted by the IR builder as a `std::vector<dagir::ir_attr>`).

Both concepts are defined against a `ReadOnlyDagView` (see `include/dagir/ro_dag_view.hpp`). Using policy objects keeps adapters and renderers decoupled.

## Common IR attribute keys

A small header `include/dagir/ir_attrs.hpp` exposes common, renderer-neutral attribute key names as `inline constexpr std::string_view` constants. Use these constants in policy objects to avoid typos and clarify intent.

Common keys and intended usages:

- `dagir::ir_attrs::kLabel` — Node or edge label text. Backends render this as visible text.
- `dagir::ir_attrs::kTooltip` — Short help text or title used as a tooltip.
- `dagir::ir_attrs::kColor` — Stroke/edge color or text color.
- `dagir::ir_attrs::kFillColor` — Node fill color.
- `dagir::ir_attrs::kStyle` — Styling hint (e.g., "dashed", "solid").
- `dagir::ir_attrs::kShape` — Node shape hint (e.g., "box", "circle").
- `dagir::ir_attrs::kPenWidth` — Stroke width for edges.
- `dagir::ir_attrs::kFontName` — Font family name.
- `dagir::ir_attrs::kFontSize` — Font size hint.
- `dagir::ir_attrs::kWeight` — Weighting hint (layout influence).
- `dagir::ir_attrs::kDir` — Edge direction hint (e.g., "forward", "both").
- `dagir::ir_attrs::kId` — An explicit identifier (if backend needs it).
- `dagir::ir_attrs::kWidth`, `dagir::ir_attrs::kHeight` — Size hints for nodes.
- `dagir::ir_attrs::kGroup` — Logical grouping name for layout or clustering.
- `dagir::ir_attrs::kGraphLabel` — Graph-level label (use on `ir_graph::global_attrs`).

Notes:
- The keys are intentionally generic. Backends can map them to renderer-specific names.
- `ir_attrs` are `std::string_view` constants; `dagir::ir_attr` uses `std::string` for keys/values. Construct `ir_attr` from the constant when emitting (e.g., `dagir::ir_attr{
  std::string(dagir::ir_attrs::kLabel), "value"}`).

## Implementing a `NodeLabeler`

A `NodeLabeler` is any callable compatible with the `NodeLabeler` concept (see `ro_dag_view.hpp`). The simplest form receives `(const View&, const Handle&)` and returns a `std::string`.

Example: stable-key stringifier

```cpp
#include <dagir/ir.hpp>
#include <string>

// A simple node labeler that uses the handle's stable_key().
struct StableKeyLabeler {
  template <class View>
  std::string operator()(const View&, const typename View::handle& h) const {
    return std::to_string(h.stable_key());
  }
};
```

    SPDX-License-Identifier: MIT

    # Implementing Policy Objects for DagIR

    This document explains how to implement policy objects that produce node labels
    and edge attributes for the DagIR `ir_graph`. It also documents the common IR
    attribute names provided by `dagir::ir_attrs` and the intended usage for
    backends (JSON, Mermaid, GraphViz, etc.).

    ## Overview

    DagIR separates traversal from rendering via small policy objects:

    - `NodeLabeler` — callable used to produce a `std::string` label for a node.
    - `EdgeAttributor` — callable used to produce attributes for an edge (return
      type interpreted by the IR builder as a `std::vector<dagir::ir_attr>`).

    Both concepts are defined against a `dagir::read_only_dag_view` (see
    `include/dagir/ro_dag_view.hpp`). Using policy objects keeps adapters and
    renderers decoupled.

    ## Common IR attribute keys

    A small header `include/dagir/ir_attrs.hpp` exposes common, renderer-neutral
    attribute key names as `inline constexpr std::string_view` constants. Use these
    constants in policy objects to avoid typos and clarify intent.

    Common keys and intended usages:

    - `dagir::ir_attrs::kLabel` — Node or edge label text. Backends render this as visible text.
    - `dagir::ir_attrs::kTooltip` — Short help text or title used as a tooltip.
    - `dagir::ir_attrs::kColor` — Stroke/edge color or text color.
    - `dagir::ir_attrs::kFillColor` — Node fill color.
    - `dagir::ir_attrs::kStyle` — Styling hint (e.g., "dashed", "solid").
    - `dagir::ir_attrs::kShape` — Node shape hint (e.g., "box", "circle").
    - `dagir::ir_attrs::kPenWidth` — Stroke width for edges.
    - `dagir::ir_attrs::kFontName` — Font family name.
    - `dagir::ir_attrs::kFontSize` — Font size hint.
    - `dagir::ir_attrs::kWeight` — Weighting hint (layout influence).
    - `dagir::ir_attrs::kDir` — Edge direction hint (e.g., "forward", "both").
    - `dagir::ir_attrs::kId` — An explicit identifier (if backend needs it).
    - `dagir::ir_attrs::kWidth`, `dagir::ir_attrs::kHeight` — Size hints for nodes.
    - `dagir::ir_attrs::kGroup` — Logical grouping name for layout or clustering.
    - `dagir::ir_attrs::kGraphLabel` — Graph-level label (use on `ir_graph::global_attrs`).

    Notes:

    - The keys are intentionally generic. Backends can map them to renderer-specific names.
    - `ir_attrs` are `std::string_view` constants; `dagir::ir_attr` uses `std::string` for keys/values. Construct `ir_attr` from the constant when emitting (e.g., `dagir::ir_attr{std::string(dagir::ir_attrs::kLabel), "value"}`).

    ## Implementing a `NodeLabeler`

    A `NodeLabeler` is any callable compatible with the `NodeLabeler` concept (see
    `ro_dag_view.hpp`). The simplest form receives `(const View&, const Handle&)`
    and returns a `std::string`.

    Example: stable-key stringifier

    ```cpp
    #include <dagir/ir.hpp>

    // A simple node labeler that uses the handle's stable_key().
    struct StableKeyLabeler {
      template <class View>
      std::string operator()(const View&, const typename View::handle& h) const {
        return std::to_string(h.stable_key());
      }
    };
    ```

    Example: custom labeler that includes family info

    ```cpp
    #include <dagir/ir.hpp>

    struct FancyLabeler {
      template <class View>
      std::string operator()(const View&, const typename View::handle& h) const {
        if constexpr (requires { h.name(); }) {
          return std::string("N_") + h.name();
        } else {
          return std::string("N_") + std::to_string(h.stable_key());
        }
      }
    };
    ```

    ## Implementing an `EdgeAttributor`

    `EdgeAttributor` callables are flexible. The `build_ir` helper in
    `include/dagir/build_ir.hpp` accepts various signatures (including forms that
    take `(view, parent, edge_like)` or `(parent, child)`), and the return type is
    interpreted as a `std::vector<dagir::ir_attr>`.

    Example: simple attribute emitter using the `ir_attrs` keys

    ```cpp
    #include <dagir/ir.hpp>
    #include <dagir/ir_attrs.hpp>
    #include <vector>

    struct SimpleEdgeAttr {
      template <class View>
      std::vector<dagir::ir_attr> operator()(const View&, const typename View::handle& p,
                                             const typename View::handle& c) const {
        std::vector<dagir::ir_attr> out;
        out.push_back({std::string(dagir::ir_attrs::kLabel), std::to_string(c.stable_key())});
        out.push_back({std::string(dagir::ir_attrs::kColor), "#ff9900"});
        return out;
      }
    };
    ```

    Best practices

    - Prefer using `dagir::ir_attrs` constants rather than string literals.
    - Keep policy objects lightweight and `noexcept`-friendly if possible.
    - Avoid heavy allocations in hot paths; labelers/attributors can cache small strings if needed.
    - Let backends interpret attribute semantics — keep keys generic.

    ## Backend mapping

    Backends map these generic keys to renderer-specific fields:

    - GraphViz: `kFillColor` -> `fillcolor`, `kPenWidth` -> `penwidth`, etc.
    - Mermaid: may map `kLabel` -> node text and use `kStyle` for class/style.
    - JSON: keep keys as-is so downstream consumers can interpret them.

    When implementing a backend, document the mapping used so policy authors can target the desired visual effect.

    ---

    This document provides a minimal, practical approach that keeps the IR and policy objects decoupled from renderer concerns while giving a stable set of attribute keys to emit and consume.
