```mermaid
graph TB
  node000("p")
  node001("q")
  node002("r")
  node003("s")
  node004["0"]
  style node004 fill:lightgray
  node005["1"]
  style node005 fill:lightgray
  node000 --> node001
  node000 --> node005
  node001 --> node002
  node001 --> node005
  node002 --> node003
  node002 --> node005
  node003 --> node004
  node003 --> node005
```
