SPDX-License-Identifier: MIT

---
SPDX-License-Identifier: MIT

## Implementing Policy Objects for DagIR

This document explains how to implement small, renderer-neutral policy objects
that produce node labels and edge attributes for the DagIR `ir_graph`. It also
documents the canonical IR attribute keys provided by `dagir::ir_attrs` and
shows examples that policy authors can copy-paste.

### Overview

DagIR separates traversal from rendering via lightweight policy objects:

- `node_labeler`: callable that returns a `std::string` label for a node.
- `edge_attributor`: callable that returns a sequence of `dagir::ir_attr`
  entries for an edge.

Both concepts target types that model `dagir::concepts::read_only_dag_view`.
Keep policy objects decoupled from backends: emit generic keys from
`dagir::ir_attrs` and let renderers map those keys to renderer-specific names.

### Canonical IR attribute keys

The header `include/dagir/ir_attrs.hpp` provides a small set of renderer-neutral
keys as `inline constexpr std::string_view` constants. Policy objects should use
these constants (snake_case) to avoid typos and make intent explicit.

Common keys and intended usages:

- `dagir::ir_attrs::k_label` — Node or edge label text.
- `dagir::ir_attrs::k_tooltip` — Tooltip/hover text.
- `dagir::ir_attrs::k_color` — Stroke/edge or text color.
- `dagir::ir_attrs::k_fill_color` — Node fill color (GraphViz: `fillcolor`).
- `dagir::ir_attrs::k_style` — Styling hints (e.g., "dashed", "solid").
- `dagir::ir_attrs::k_shape` — Node shape hint (e.g., "box", "circle").
- `dagir::ir_attrs::k_pen_width` — Edge stroke width (GraphViz: `penwidth`).
- `dagir::ir_attrs::k_font_name` — Font family name.
- `dagir::ir_attrs::k_font_size` — Font size hint.
- `dagir::ir_attrs::k_weight` — Layout-weight hint.
- `dagir::ir_attrs::k_dir` — Edge direction hint (e.g., "forward").
- `dagir::ir_attrs::k_id` — Explicit identifier when required.
- `dagir::ir_attrs::k_width`, `dagir::ir_attrs::k_height` — Size hints.
- `dagir::ir_attrs::k_group` — Logical grouping name (clustering/layout).
- `dagir::ir_attrs::k_graph_label` — Graph-level label (attach to `ir_graph::global_attrs`).

Notes:

- Keys are generic; backends map them to renderer-specific fields.
- `dagir::ir_attrs` are `std::string_view` constants; construct `dagir::ir_attr`
  instances using `std::string(dagir::ir_attrs::k_label)` for the key when
  emitting attributes.

### Implementing a `node_labeler`

A `node_labeler` is any callable compatible with the `dagir::concepts::node_labeler`
concept. The implementation may accept either `(const View&, handle)` or just
`(handle)` (see the concept documentation for the exact accepted forms).

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

### Implementing an `edge_attributor`

`edge_attributor` callables are flexible. The `build_ir` helper accepts several
call signatures (e.g., `(view, parent, edge_like)`, `(parent, child)`) and
interprets the result as a `std::vector<dagir::ir_attr>`.

Example: simple attribute emitter using `dagir::ir_attrs`

```cpp
#include <dagir/ir.hpp>
#include <dagir/ir_attrs.hpp>
#include <vector>

struct SimpleEdgeAttr {
  template <class View>
  std::vector<dagir::ir_attr> operator()(const View&, const typename View::handle& p,
                                         const typename View::handle& c) const {
    std::vector<dagir::ir_attr> out;
    out.push_back({std::string(dagir::ir_attrs::k_label), std::to_string(c.stable_key())});
    out.push_back({std::string(dagir::ir_attrs::k_color), "#ff9900"});
    return out;
  }
};
```

### Best practices

- Prefer `dagir::ir_attrs` constants over string literals.
- Keep policy objects lightweight and `noexcept`-friendly when possible.
- Avoid heavy allocations on hot paths; cache small strings if needed.
- Let backends interpret keys; document mappings when creating a renderer.

### Backend mapping

Backends map the generic keys to renderer-specific names (examples):

- GraphViz: map `k_fill_color` -> `fillcolor`, `k_pen_width` -> `penwidth`.
- Mermaid: map `k_label` -> node text and `k_style` -> classes/styles.
- JSON: keep keys as-is for downstream consumers.

---

This document aims to provide a concise, copy-pasteable reference for policy
authors. If you'd like example labelers/attributors for common frameworks or a
small policy test harness, I can add them here.
