```mermaid
graph TB
  node000["AND"]
  style node000 fill:lightgreen
  node001["XOR"]
  style node001 fill:lightpink
  node002["OR"]
  style node002 fill:lightcoral
  node003["x"]
  style node003 fill:lightblue
  node004["y"]
  style node004 fill:lightblue
  node005["u"]
  style node005 fill:lightblue
  node006["v"]
  style node006 fill:lightblue
  node000 -- "L" --> node001
  node000 -- "R" --> node002
  node001 -- "L" --> node003
  node001 -- "R" --> node004
  node002 -- "L" --> node005
  node002 -- "R" --> node006
```
