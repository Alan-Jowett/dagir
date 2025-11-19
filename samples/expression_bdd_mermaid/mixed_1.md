```mermaid
graph TB
  node000("a")
  node001("b")
  node002("c")
  node003["0"]
  style node003 fill:lightgray
  node004["1"]
  style node004 fill:lightgray
  node000 --> node001
  node000 --> node003
  node001 --> node002
  node001 --> node004
  node002 --> node003
  node002 --> node004
```
