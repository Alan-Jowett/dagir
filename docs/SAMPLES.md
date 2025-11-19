<!-- SPDX-License-Identifier: MIT
  Copyright (c) 2025 DagIR contributors -->
## Sample Applications

This document summarizes the sample applications included in the repository and how to build/run them.

- `samples/expression2tree`
  - Purpose: parse a textual logical expression, build an expression AST, run DagIR to build an `ir_graph`, and render the expression tree using a chosen backend (DOT, JSON, or Mermaid).
  - Key files: `samples/expression2tree/main.cpp` and helpers under `samples/include/expressions`.
  - Usage: `expression2tree <expression_file> [backend]` where `backend` is `dot` (default), `json`, or `mermaid`.

- `samples/expression2bdd`
  - Purpose: parse an expression, convert to a BDD using either the Teddy or CUDD library, expose the BDD as a `read_only_dag_view`, build an `ir_graph` via DagIR, and render the BDD IR.
  - Key files: `samples/expression2bdd/main.cpp`, `samples/include/teddy/*`, and `samples/include/cudd/*`.
  - Usage: `expression2bdd <expression_file> <library> <backend>` where `library` is `teddy` or `cudd`, and `backend` is `dot|json|mermaid`.

Notes and prerequisites
- The sample apps are small CLI programs that depend on the header-only DagIR library in `include/dagir` and the helper headers in `samples/include`.
- The `expression2bdd` sample optionally depends on third-party BDD libraries:
  - Teddy: the repository includes sample Teddy helpers, but a real build will require linking the Teddy library if you want to run the binary.
  - CUDD: building/running with `cudd` requires installed CUDD development headers and libraries.
- The `mermaid` backend writes fenced Markdown with a Mermaid block (```mermaid ... ```) suitable for embedding in Markdown viewers.
- To build the samples with CMake (recommended):

```powershell
mkdir build
cd build
cmake -S .. -B . -DCMAKE_BUILD_TYPE=Release
cmake --build . --target expression2tree --config Release
cmake --build . --target expression2bdd --config Release
```

- Example expression files are provided under `samples/expressions/*.expr` and JSON/mermaid/dot sample outputs are in the `samples/` subfolders.

If you'd like, I can add small CMake targets or README snippets per-sample to make running them locally easier.
