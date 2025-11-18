<!-- SPDX-License-Identifier: MIT
  Copyright (c) 2025 DagIR contributors -->

# Copilot Notes — discoveries and helpful commands

These notes capture details learned while working on the DOT renderer and tests.


Build & tests
- To build tests you must enable the CMake option `DAGIR_BUILD_TESTS=ON` when configuring.
  - Example (from repo root on Windows):

```powershell
cmake -S . -B build -DDAGIR_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --config RelWithDebInfo --target ALL_BUILD
```

- On Windows/MSVC CMake is multi-config; `CMAKE_BUILD_TYPE` is ignored by the generator (harmless warning).
- If Catch2 is not found, CMake's FetchContent will populate `build/_deps/catch2-*` during configure/build.
- The test executable target name is `dagir_tests` (created when test sources are present). CTest entries are registered via `catch_discover_tests`.
- The project install step in configure places a `pre-commit` hook at `.git/hooks/pre-commit` (developer convenience).

Running tests
- After building, run the test binary directly (recommended on Windows):

```powershell
./build/RelWithDebInfo/dagir_tests.exe -s
```

Renderer implementation notes (`include/dagir/render_dot.hpp`)
- Header-only renderer `dagir::render_dot(std::ostream&, const dagir::ir_graph&, std::string_view)` added.
- Behavior and mapping decisions:
	- Always emits a `digraph` named by the `graph_name` parameter.
	- Nodes are emitted with identifiers `n{id}` where `id` is `ir_node::id`.
	- Node label selection order: `ir_attr` with key `k_label` → numeric id.
	- Edge labels are taken from `k_label` if present.
	- If a node has `k_fill_color` and no explicit `k_style`, `style=filled` is emitted.
	- Attribute keys are emitted as provided (the renderer relies on keys in `include/dagir/ir_attrs.hpp` to be GraphViz-appropriate names like `fillcolor`, `penwidth`, `fontsize`, etc.).
	- Values are quoted and escaped (quotes, backslashes, newlines handled).

Testing the renderer
- A unit test `tests/test_render_dot.cpp` was added that constructs a tiny `ir_graph`, renders it to an `ostringstream`, and asserts presence of expected snippets (graph header, node labels, edge, style filled, etc.).
- All local tests passed after adding the file.

Follow-ups you might want
- Map more `ir_attrs` to GraphViz canonical names explicitly (e.g., map `k_pen_width` &rarr; `penwidth`, `k_font_size` &rarr; `fontsize`) and emit numeric values unquoted when appropriate.
- Add support for undirected graphs, clusters/subgraphs, or port labels if needed.
- Run `clang-format` on the new header and commit formatting changes to keep the repository consistent with the project's formatting rules.

Helpful commands recap

```powershell
# Configure with tests enabled
cmake -S . -B build -DDAGIR_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo
# Build everything
cmake --build build --config RelWithDebInfo --target ALL_BUILD
# Run tests
./build/RelWithDebInfo/dagir_tests.exe -s
```

<!--
SPDX-License-Identifier: MIT
Copyright (c) 2025 DagIR contributors
-->
This repository uses GitHub Copilot and the GitHub Copilot coding agent guidance.

Guidance for contributors:
- Keep responses concise and focused on code changes.
- Do not commit generated large transformations without a human review.
- When asked to modify repository files, ensure tests still pass locally.
 - Prefer small, targeted fixes: keep each change focused and minimal to address a single concern.
 
Additional requirements for automated agents (Copilot/coding agents):
- All commits produced or suggested by automated agents must include a Signed-off-by line (`-s`/`Signed-off-by:`) in the commit message. Do not create commits without this sign-off.
- Before creating or committing any changes, the agent must build the project and run the test suite locally. If the build or tests fail, the agent should not commit the changes and must instead provide the failing output and suggested fixes.
- If automated fixes are applied (formatting, linting), the agent should re-run build and tests before committing.

If you are a maintainer, include these steps in PR descriptions when asking Copilot to modify files:
- Which files to change.
- A concise spec of why the change is needed.
- Any tests or formatting checks to run after changes.
 
When submitting PRs, include a short checklist in the PR body confirming:
- Commits are signed-off.
- Build completed locally (tool and command used).
- Tests executed and passed (list of failing tests if any).
