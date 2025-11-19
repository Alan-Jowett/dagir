```mermaid
graph TB
  node000["AND"]
  style node000 fill:lightgreen
  node001["NOT"]
  style node001 fill:yellow
  node002["OR"]
  style node002 fill:lightcoral
  node003["a"]
  style node003 fill:lightblue
  node004["b"]
  style node004 fill:lightblue
  node005["c"]
  style node005 fill:lightblue
  node000 -- "L" --> node001
  node000 -- "R" --> node002
  node001 --> node003
  node002 -- "L" --> node004
  node002 -- "R" --> node005
```
