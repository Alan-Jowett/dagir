```mermaid
graph TB
  node1["OR"]
  style node1 fill:lightcoral
  node2["OR"]
  style node2 fill:lightcoral
  node3["s"]
  style node3 fill:lightblue
  node4["OR"]
  style node4 fill:lightcoral
  node5["r"]
  style node5 fill:lightblue
  node6["p"]
  style node6 fill:lightblue
  node7["q"]
  style node7 fill:lightblue
  node1 -- "L" --> node2
  node1 -- "R" --> node3
  node2 -- "L" --> node4
  node2 -- "R" --> node5
  node4 -- "L" --> node6
  node4 -- "R" --> node7
```
