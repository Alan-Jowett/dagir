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
- **Concepts**: `ExternalDagView`, `NodeHandle`, `EdgeRef`.
- **Algorithms**:
  - `topo_order(view, roots)` – Kahn’s algorithm.
  - `postorder_fold(view, root, combine)` – N-ary fold with memoization.
- **IR Layer**:
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
#include <dagir/dag_ir.hpp>
#include <dagir/topo_order.hpp>
#include <dagir/render_dot.hpp>

using View = TeddyView<MyTeddyTraits>;
View G{ &mgr, { View::handle{root_node} } };

auto ir = build_ir(G, DotPolicy{&G, &ctx});
render_dot(ir, std::cout);
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
- [ ] Mermaid renderer
- [ ] JSON renderer
- [ ] Parallel traversal
- [ ] Layout integration (Graphviz or drag)

---

## ✅ License
MIT License – free for commercial and OSS use.

---

## 🤝 Contributing
PRs welcome! Please see CONTRIBUTING.md for guidelines.

---

## 📚 Why DagIR?
Because you shouldn’t have to copy your DAG just to visualize or analyze it.
