```mermaid
%%{ init: {"theme": "default"} }%%
graph TB
  node000("m")
  node001("n")
  node002("n")
  node003["1"]
  style node003 fill:lightgray
  node004["0"]
  style node004 fill:lightgray
  node000 --> node001
  node000 --> node002
  node001 --> node003
  node001 --> node004
  node002 --> node003
  node002 --> node004
```
