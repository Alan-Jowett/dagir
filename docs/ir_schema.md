# DagIR IR JSON schema
<!-- SPDX-License-Identifier: MIT
  Copyright (c) 2025 DagIR contributors -->

Top-level object:

- `schema_version`: integer (required). Current value: `1`.
- `nodes`: array of node objects (required).
- `edges`: array of edge objects (required).
- `graphAttributes`: object (optional).
- `roots`: array of string node ids (optional).

Node object:

- `id`: string (required). Prefer `name` strings where available; numeric strings are accepted.
- `label`: string (optional).
- `attributes`: object (optional) — arbitrary key/value metadata. Values may be primitives or strings.
  - `cycle_group`: string (optional) — identifier for the strongly connected component (SCC) this node belongs to. Nodes with the same `cycle_group` value are part of the same cycle in a directed cyclic graph (DCG).

Edge object:

- `source`: string (required) — node id.
- `target`: string (required) — node id.
- `attributes`: object (optional).
  - `is_cycle_back_edge`: string (optional) — when set to "true", indicates this edge is a back-edge that creates a cycle in the graph.

Graph attributes:

- `graph.has_cycles`: string (optional) — when set to "true", indicates the graph contains one or more cycles (is a DCG). When "false" or absent, the graph is acyclic (DAG).

Notes:
- `from_json` will parse node `id` strings as integers when possible and assign numeric `ir_node::id` values. When the original `id` is non-numeric it is preserved in the node attributes under `dagir::ir_attrs::k_id`.
- Attribute values are stored in the IR as strings; types are preserved in the JSON representation but converted to string form in the `ir_graph` and cached in `ir_graph::attr_cache`.
- Cycle-related attributes (`cycle_group`, `is_cycle_back_edge`, `graph.has_cycles`) are optional and only present when the graph contains cycles. These attributes are populated when `build_ir` is called with the `detect_cycles_flag` parameter set to `true`.

Example JSON (DAG):

```json
{
  "schema_version": 1,
  "nodes": [
    { "id": "root", "label": "Root Node", "attributes": { "role": "entry" } }
  ],
  "edges": []
}
```

Example JSON (DCG with cycle):

```json
{
  "schema_version": 1,
  "graphAttributes": {
    "graph.has_cycles": "true"
  },
  "nodes": [
    { "id": "0", "label": "Node A", "attributes": { "cycle_group": "0" } },
    { "id": "1", "label": "Node B", "attributes": { "cycle_group": "0" } }
  ],
  "edges": [
    { "source": "0", "target": "1", "attributes": {} },
    { "source": "1", "target": "0", "attributes": { "is_cycle_back_edge": "true" } }
  ]
}
```
