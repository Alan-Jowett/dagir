```mermaid
graph TB
  node1["AND"]
  style node1 fill:lightgreen
  node2["XOR"]
  style node2 fill:lightpink
  node3["OR"]
  style node3 fill:lightcoral
  node4["x"]
  style node4 fill:lightblue
  node5["y"]
  style node5 fill:lightblue
  node6["u"]
  style node6 fill:lightblue
  node7["v"]
  style node7 fill:lightblue
  node1 -- "L" --> node2
  node1 -- "R" --> node3
  node2 -- "L" --> node4
  node2 -- "R" --> node5
  node3 -- "L" --> node6
  node3 -- "R" --> node7
```
