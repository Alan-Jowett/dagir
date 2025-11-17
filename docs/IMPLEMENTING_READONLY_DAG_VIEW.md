SPDX-License-Identifier: MIT

# Implementing `read_only_dag_view`

This document explains how to implement a `read_only_dag_view` adapter for DagIR. A
`read_only_dag_view` is a lightweight, non-owning view over an existing DAG and
exposes a minimal API that the DagIR algorithms and `build_ir` use to traverse
nodes and edges without copying the underlying graph.

See `include/dagir/concepts/read_only_dag_view.hpp` for the formal concept definitions; this
document provides a practical summary and examples.

## Requirements (concept summary)

A type models `dagir::read_only_dag_view` when it satisfies the following:

- Defines `using handle = ...;` and that `handle` models `dagir::node_handle`.
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


### `NodeHandle` requirements

A `NodeHandle` must be `std::copyable` and provide:

- `stable_key()` -> convertible to `std::uint64_t` (a stable identifier suitable for memoization)
- `debug_address()` -> convertible to `const void*` (optional diagnostic pointer)
- equality/inequality operators (`==` and `!=`)

`stable_key()` must be stable across the lifetime of objects used in traversal so the IR builder can memoize nodes consistently.

## Implementing the minimal adapter: example

This example implements a tiny in-memory DAG view suitable for tests and demos. It uses integer node ids and a simple `BasicEdge` adapter.

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

    Notes on the example : - `children` returns a range whose value_type is the `handle` type;
that satisfies `dagir::ChildrenRange`.- `roots()` returns a range of
                                        handles(we used `std::views` to create it lazily)
                                            .

                                        ##Implementing an `EdgeRef`

                                        If your backend needs edges that carry labels
    or other metadata,
    implement an `EdgeRef` type exposing `target()`.

```cpp struct EdgeRefImpl {
  IntHandle to;
  std::string label;
  constexpr const IntHandle& target() const noexcept { return to; }
  const std::string& get_label() const& { return label; }
};
```

    Then make `children(handle)` return a range of `EdgeRefImpl` objects.

    ##Optional : `start_guard`

    If your backend
      requires pinning or a
    scoped lock during traversal,
    provide `start_guard(handle)` returning an RAII guard type
        .The `build_ir` implementation will call it when present.

    Example :

```cpp struct PinGuard {
  PinGuard(const IntHandle&) { /* pin node */ }
  ~PinGuard() { /* unpin node */ }
};

struct SomeView {
  using handle = IntHandle;
  PinGuard start_guard(const handle& h) const { return PinGuard(h); }
};
```

`build_ir` will call `view.start_guard(h)` when the expression is well
    - formed.

      ##Policy examples

      Once you have a `ReadOnlyDagView`,
    you can write policy objects(labelers, attributors) that accept the view and handles. See `docs/IMPLEMENTING_POLICY.md` for examples that use `dagir::ir_attrs` keys.

## Debugging tips

- If `build_ir` fails to compile, check that your `children()` return type satisfies `dagir::ChildrenRange` — it must be an input-range whose value_type or reference_type provides `target()` (or is convertible to `handle`).
- Ensure `stable_key()` returns a stable, unique id for each logical node. Collisions will break memoization.
- Use `dagir::NoopGuard` when you don't need special guarding semantics.

## Summary

Implementing a `ReadOnlyDagView` is intentionally lightweight:
- Provide `using handle = ...` where `handle` models `NodeHandle`.
- Provide `children(handle)` and `roots()` ranges.
- Optionally provide `start_guard(handle)` if needed.

With these in place you can use DagIR traversal algorithms and `build_ir` to emit an `ir_graph` for rendering or analysis.
