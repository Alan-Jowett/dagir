<!-- SPDX-License-Identifier: MIT
  Copyright (c) 2025 DagIR contributors -->

# Implementing `read_only_dag_view`

This document explains how to implement a `read_only_dag_view` adapter for DagIR. A
`read_only_dag_view` is a lightweight, non-owning view over an existing graph and
exposes a minimal API that the DagIR algorithms and `build_ir` use to traverse
nodes and edges without copying the underlying graph.

**Note**: Despite the name `read_only_dag_view`, DagIR now supports both directed
acyclic graphs (DAGs) and directed cyclic graphs (DCGs). The concept name is
retained for backward compatibility. When using `build_ir` with the
`detect_cycles_flag=true` parameter, cyclic graphs are properly handled and
cycle metadata is added to the IR.

See `include/dagir/concepts/read_only_dag_view.hpp` for the formal concept definitions; this
document provides a practical summary and examples.

## Requirements (concept summary)

A type models `dagir::concepts::read_only_dag_view` when it satisfies the following:

- Defines `using handle = ...;` and that `handle` models `dagir::concepts::node_handle`.
- Provides `children(handle)` returning an input-range of child "edge-like"
  elements; each element is either a child handle or an `edge_ref` type that
  exposes `target() -> handle`.
- Provides `roots()` returning an input-range of `handle` values.

Optional:
- `start_guard(handle)` returning an RAII guard type used to pin resources
  during traversal. Adapters that don't require pinning can return
  `dagir::noop_guard`.

## Example minimal adapter

```cpp
#include <dagir/concepts/read_only_dag_view.hpp>

struct MyHandle { uint64_t key; uint64_t stable_key() const noexcept { return key; } };

struct MyView {
  using handle = MyHandle;
  // returns an input-range of MyHandle for children
  auto children(const handle& h) const -> std::vector<MyHandle>;
  // returns an input-range of roots
  auto roots() const -> std::vector<MyHandle>;
};
```

With these in place you can use DagIR traversal algorithms and `build_ir` to
emit an `ir_graph` for rendering or analysis.


### `node_handle` requirements

A `node_handle` must be `std::copyable` and provide:

- `stable_key()` -> convertible to `std::uint64_t` (a stable identifier suitable for memoization)
- `debug_address()` -> convertible to `const void*` (optional diagnostic pointer)
- equality/inequality operators (`==` and `!=`)

`stable_key()` must be stable across the lifetime of objects used in traversal so the IR builder can memoize nodes consistently.

## Implementing the minimal adapter: example

This example implements a tiny in-memory DAG view suitable for tests and demos. It uses integer node ids and a simple `basic_edge` adapter.

```cpp
#include <cstdint>
#include <dagir/concepts/read_only_dag_view.hpp>
#include <string>
#include <vector>

struct IntHandle {
  std::uint64_t id;
  constexpr std::uint64_t stable_key() const noexcept { return id; }
  constexpr const void* debug_address() const noexcept { return nullptr; }
  constexpr bool operator==(const IntHandle& o) const noexcept { return id == o.id; }
  constexpr bool operator!=(const IntHandle& o) const noexcept { return id != o.id; }
};

struct IntDagView {
  using handle = IntHandle;
  std::vector<std::vector<IntHandle>> adjacency;  // adjacency list

  // roots: nodes with no incoming edges (for this example we return 0..N-1)
  auto roots() const {
    return std::views::iota(0ull, adjacency.size()) |
           std::views::transform([](auto i) { return IntHandle{i}; });
  }

  // children: return a range of IntHandle values
  auto children(const IntHandle& h) const {
    return adjacency[h.id] | std::views::transform([](const IntHandle& ch) { return ch; });
  }
};
```

Notes on the example:

- `children` returns a range whose `value_type` is the `handle` type; that
  satisfies `dagir::ChildrenRange`.
- `roots()` returns a lazy range of handles using `std::views`.

## Implementing an `edge_ref`

If your backend needs edges that carry labels or other metadata, implement an
`edge_ref` type exposing `target()`:

```cpp
struct EdgeRefImpl {
  IntHandle to;
  std::string label;
  constexpr const IntHandle& target() const noexcept { return to; }
  const std::string& get_label() const& { return label; }
};
```

Then make `children(handle)` return a range of `EdgeRefImpl` objects.

## Optional: `start_guard`

If your backend requires a scoped lock or pinning during traversal, provide
`start_guard(handle)` returning an RAII guard type. The `build_ir` helper will
call it when present.

Example:

```cpp
struct PinGuard {
  PinGuard(const IntHandle&) { /* pin node */ }
  ~PinGuard() { /* unpin node */ }
};

struct SomeView {
  using handle = IntHandle;
  PinGuard start_guard(const handle& h) const { return PinGuard(h); }
};
```

`build_ir` will call `view.start_guard(h)` when the expression is well-formed.

## Policy examples

Once you have a `read_only_dag_view`, write policy objects called node
attributors that accept the view and a node handle and return a
`dagir::ir_attr_map` (an attribute map). The required concept is
`dagir::concepts::node_attributor` and attribute keys are defined in
`dagir::ir_attrs`.

See `docs/IMPLEMENTING_POLICY.md` for examples and best practices. A minimal
example node attributor:

```cpp
#include <dagir/ir.hpp>       // for dagir::ir_attr_map
#include <dagir/ir_attrs.hpp> // for dagir::ir_attrs

dagir::ir_attr_map my_node_attributor(const MyView& /*view*/, const MyView::handle& h) {
  return {
    { dagir::ir_attrs::k_label, std::to_string(h.stable_key()) }
  };
}
```

## Debugging tips

- If `build_ir` fails to compile, check that your `children()` return type satisfies `dagir::ChildrenRange` — it must be an input-range whose value_type or reference_type provides `target()` (or is convertible to `handle`).
- Ensure `stable_key()` returns a stable, unique id for each logical node. Collisions will break memoization.
-- Use `dagir::noop_guard` when you don't need special guarding semantics.

## Summary

Implementing a `read_only_dag_view` is intentionally lightweight:
- Provide `using handle = ...` where `handle` models `node_handle`.
- Provide `children(handle)` and `roots()` ranges.
- Optionally provide `start_guard(handle)` if needed.

With these in place you can use DagIR traversal algorithms and `build_ir` to emit an `ir_graph` for rendering or analysis.

## Handling Cycles in Adapters

DagIR now supports directed cyclic graphs (DCGs) in addition to DAGs. Your adapter
doesn't need special handling for cycles — the library automatically detects and
processes them when you enable cycle detection.

### Using cycle detection

When calling `build_ir`, pass `true` as the fourth parameter to enable cycle detection:

```cpp
auto ir = dagir::build_ir(view, node_policy, edge_policy, true);
```

With cycle detection enabled:
- The library uses DFS traversal instead of Kahn's topological sort
- Cycles are detected using Tarjan's algorithm for finding strongly connected components
- Back-edges (edges that create cycles) are marked in the IR
- Nodes in the same cycle group (SCC) receive the same `cycle_group` attribute

### Example with cycles

```cpp
// Graph with cycle: 0 -> 1 -> 0
MockDagView cyclic_view({MockHandle{0}}, {{MockHandle{1}}, {MockHandle{0}}});

auto node_attr = [](auto const& /*view*/, auto const& h) {
  dagir::ir_attr_map m;
  m[dagir::ir_attrs::k_label] = std::to_string(h.stable_key());
  return m;
};
auto edge_attr = [](auto&&...) { return dagir::ir_attr_map{}; };

// Enable cycle detection
auto ir = dagir::build_ir(cyclic_view, node_attr, edge_attr, true);

// The IR will contain:
// - graph.has_cycles = "true" in global_attrs
// - cycle_group attribute on both nodes (same value)
// - is_cycle_back_edge = "true" on the back-edge (1 -> 0)
```

### Backward compatibility

By default, cycle detection is **disabled** (`detect_cycles_flag = false`):
- Kahn's algorithm is used (throws on cycles)
- No cycle metadata is added to the IR
- Existing code continues to work unchanged

Enable cycle detection only when you need to handle cyclic graphs.

### Cycle attributes in IR

When cycle detection is enabled, the IR contains:

**Graph-level attributes:**
- `graph.has_cycles`: Set to "true" if cycles are detected

**Node attributes:**
- `cycle_group`: Integer identifier for the strongly connected component (SCC).
  Nodes with the same value are part of the same cycle.

**Edge attributes:**
- `is_cycle_back_edge`: Set to "true" for back-edges that create cycles.

These attributes are available for renderers to visualize cycles appropriately
(e.g., using dashed lines or different colors for back-edges).
