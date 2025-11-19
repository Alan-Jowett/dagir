```mermaid
%%{ init: {"theme": "default"} }%%
graph TB
  node000["OR"]
  style node000 fill:lightcoral
  node001["AND"]
  style node001 fill:lightgreen
  node002["AND"]
  style node002 fill:lightgreen
  node003["a"]
  style node003 fill:lightblue
  node004["b"]
  style node004 fill:lightblue
  node005["c"]
  style node005 fill:lightblue
  node006["d"]
  style node006 fill:lightblue
  node000 -- "L" --> node001
  node000 -- "R" --> node002
  node001 -- "L" --> node003
  node001 -- "R" --> node004
  node002 -- "L" --> node005
  node002 -- "R" --> node006
```
