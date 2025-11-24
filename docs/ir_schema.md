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

Edge object:

- `source`: string (required) — node id.
- `target`: string (required) — node id.
- `attributes`: object (optional).

Notes:
- `from_json` will parse node `id` strings as integers when possible and assign numeric `ir_node::id` values. When the original `id` is non-numeric it is preserved in the node attributes under `dagir::ir_attrs::k_id`.
- Attribute values are stored in the IR as strings; types are preserved in the JSON representation but converted to string form in the `ir_graph` and cached in `ir_graph::attr_cache`.

Example JSON:

```json
{
  "schema_version": 1,
  "nodes": [
    { "id": "root", "label": "Root Node", "attributes": { "role": "entry" } }
  ],
  "edges": []
}
```
