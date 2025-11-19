```mermaid
%%{ init: {"theme": "default"} }%%
graph TB
  node000["AND"]
  style node000 fill:lightgreen
  node001["NOT"]
  style node001 fill:yellow
  node002["c"]
  style node002 fill:lightblue
  node003["OR"]
  style node003 fill:lightcoral
  node004["a"]
  style node004 fill:lightblue
  node005["b"]
  style node005 fill:lightblue
  node000 -- "L" --> node001
  node000 -- "R" --> node002
  node001 --> node003
  node003 -- "L" --> node004
  node003 -- "R" --> node005
```
