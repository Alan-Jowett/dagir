<!-- SPDX-License-Identifier: MIT
  Copyright (c) 2025 DagIR contributors -->

# Copilot Instructions for DagIR

**DagIR** is a header-only C++20 library for non-owning traversal of external DAGs and renderer-neutral IR generation. This guide helps AI agents understand the architecture, build system, and contribution patterns.

## Architecture & Core Concepts

**Three-tier design**: adapters → IR builder → renderers

1. **Adapters** (non-owning views) — implement `read_only_dag_view` concept
   - `include/dagir/utility/{expressions,cudd,teddy}/` contain adapters for expression ASTs, CUDD BDDs, and TeDDy diagrams
   - Each adapter provides a `handle` type, `children(handle)` method, and `roots()` method
   - Example: `expression_read_only_dag_view` wraps expression AST nodes without copying

2. **IR Builder** — `dagir::build_ir()` in `include/dagir/build_ir.hpp`
   - Traverses DAG via Kahn's topological sort
   - Applies policy callables (`node_attributor`, `edge_attributor`) to produce generic attributes
   - Stores non-owning `std::string_view` attributes in `ir_graph`

3. **Renderers** — convert `ir_graph` to target format
   - `render_dot`, `render_json`, `render_mermaid` in `include/dagir/render_*.hpp`
   - Use canonical attribute keys from `ir_attrs.hpp` (e.g., `k_label`, `k_fill_color`)
   - Renderers map generic keys to format-specific names (e.g., `k_fill_color` → GraphViz `fillcolor`)

**Key principle**: Policies are attribute-producers, not backend-specific. Use `ir_attrs::k_*` constants when implementing attributors.

## Critical Implementation Pattern: Policies

All adapters ship with node and edge attributor pairs (`*_policy.hpp`):

```cpp
// Example from expression_policy.hpp
struct expression_node_attributor {
  template <class View>
  dagir::ir_attr_map operator()(const View&, const typename View::handle& h) const {
    dagir::ir_attr_map m;
    m.emplace(dagir::ir_attrs::k_label, format_label(h));
    return m;
  }
};
```

**When adding new logic**:
- Node/edge attributes go in `*_policy.hpp`, not renderers
- Return `std::unordered_map<std::string, std::string>` and let `build_ir` cache views
- Use `dagir::ir_attrs::k_*` constants (defined in `ir_attrs.hpp`)

## Build & Test Commands

**Configure with tests enabled** (Windows):
```powershell
cmake -S . -B build -DDAGIR_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --config RelWithDebInfo --target ALL_BUILD --parallel
```

**Clean build** (remove build artifacts and reconfigure):
```powershell
Remove-Item -Recurse -Force build
cmake -S . -B build -DDAGIR_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --config RelWithDebInfo --target ALL_BUILD --parallel
```

**Run tests via CTest** (from the build directory on Windows):
```powershell
cd build
ctest -C RelWithDebInfo --parallel
```

**Key points**:
- Tests require `DAGIR_BUILD_TESTS=ON` (Catch2 v3 auto-fetched via FetchContent if missing)
- On Windows/MSVC, `cd` into the build directory first, then run CTest with `-C RelWithDebInfo`
- On Linux/macOS, run CTest from the workspace root: `ctest -C RelWithDebInfo --output-on-failure --verbose`
- Use `--parallel` flag with `cmake --build` for faster parallel builds (uses all available CPU cores)
- On Windows/MSVC, `CMAKE_BUILD_TYPE` is ignored (multi-config generator); executables output to `build/<CONFIG>/` subdirectories
- CI matrix runs tests on Linux (gcc/clang), macOS (gcc/clang), Windows (MSVC) across Debug/Release builds
- Sanitizer tests (ASAN/UBSAN) run separately on Linux with environment variable configuration
- On macOS, TeDDy-dependent tests (`test_equivalence_*.cpp`) are skipped due to platform incompatibilities
- Pre-commit hook installed at `.git/hooks/pre-commit` during configure

## Formatting & Submission

- Code style defined in `.clang-format`; run `clang-format -i <file>` before committing
- CI includes formatting check (`.github/workflows/format-and-cppcheck.yml`) and cppcheck
- Sanitizers (ASAN/UBSAN) available via CMake options:
  ```powershell
  cmake -S . -B build -DENABLE_ASAN=ON -DENABLE_UBSAN=ON
  ```

## Automated Agent Requirements

- **All commits must include `Signed-off-by`** line (`git commit -s`)
- **Build and test locally before committing**:
  1. Configure with `DAGIR_BUILD_TESTS=ON`
  2. Build and run full test suite
  3. If build/tests fail, provide output and suggest fixes (do not commit)
- **Small, focused changes**: one concern per commit
- **Post-format checks**: re-run build/tests after applying clang-format

## File References & Navigation

| Purpose | Location |
|---------|----------|
| Concepts (traits) | `include/dagir/concepts/` |
| Algorithms (Kahn, postorder fold) | `include/dagir/algorithms.hpp` |
| IR types & schema | `include/dagir/ir.hpp`, `docs/ir_schema.md` |
| Canonical attributes | `include/dagir/ir_attrs.hpp` |
| Example adapters & policies | `include/dagir/utility/{expressions,cudd,teddy}/` |
| Renderer examples | `include/dagir/render_dot.hpp` (simple; start here) |
| Policy guide | `docs/IMPLEMENTING_POLICY.md` |
| Adapter guide | `docs/IMPLEMENTING_READONLY_DAG_VIEW.md`
