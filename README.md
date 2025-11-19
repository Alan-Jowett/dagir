<!-- SPDX-License-Identifier: MIT
  Copyright (c) 2025 DagIR contributors -->
# DagIR
**A header-only C++20 library for external DAG traversal, IR generation, and multi-backend rendering.**

---

## ✨ Why DagIR?
Existing graph libraries assume you own the graph. DagIR is different:
- Works on **external DAGs** (TeDDy, CUDD, expression DAGs) without copying.
- Provides a **renderer-neutral IR** for DOT, Mermaid, JSON, or custom backends.
- Uses **policy-driven customization** for labels, styles, and metadata.
- Lightweight, **header-only**, and **MIT licensed**.

---

## ✅ Features
 **Concepts**: `read_only_dag_view`, `node_handle`, `edge_ref`.
   - `kahn_topological_order(view)` – Kahn’s algorithm.
  - `postorder_fold(view, combiner)` – N-ary fold with memoization (returns map from node `stable_key()` to result).
   - `ir_graph` with nodes, edges, attributes.
- **Renderers**:
  - DOT (Graphviz)
  - Mermaid
  - JSON
- **Adapters**:
  - TeDDy
  - CUDD
  - Mock for testing.

---

## 🚀 Quick Start
```cpp
#include <dagir/ir.hpp>
#include <dagir/algorithms.hpp>
#include <dagir/build_ir.hpp>

using View = TeddyView<MyTeddyTraits>;
View G{ &mgr, { View::handle{root_node} } };

// Build the intermediate representation using policies, then render.
auto ir = dagir::build_ir(G, DotPolicy{&G, &ctx});
// Rendering helpers are in the renderers directory; if you have a DOT
// renderer available it will take an `ir_graph` and produce DOT output.
// render_dot(ir, std::cout);
```

---

## 📦 Installation
Header-only:
```bash
git clone https://github.com/your-org/dagir.git
```
Add `include/dagir` to your include path.

---

## 🛠 Roadmap
- [x] Core concepts and algorithms
- [x] DOT renderer
- [x] Mermaid renderer
- [x] JSON renderer
- [ ] Parallel traversal
- [ ] Layout integration (Graphviz or drag)

---

## ✅ License
MIT License – free for commercial and OSS use.

---

## 🤝 Contributing
PRs welcome! Please see CONTRIBUTING.md for guidelines.

---

## Continuous Integration
This repository now includes GitHub Actions workflows to run sanitizer builds (ASAN/UBSAN) and collect coverage using `lcov`/Codecov. See `.github/workflows/main.yml` (sanitizer-tests job) and `.github/workflows/coverage.yml`.


## 📚 Why DagIR?
Because you shouldn’t have to copy your DAG just to visualize or analyze it.
