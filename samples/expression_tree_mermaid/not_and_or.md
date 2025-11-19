```mermaid
%%{ init: {"theme": "default"} }%%
graph TB
  node000["OR"]
  style node000 fill:lightcoral
  node001["NOT"]
  style node001 fill:yellow
  node002["AND"]
  style node002 fill:lightgreen
  node003["x"]
  style node003 fill:lightblue
  node004["y"]
  style node004 fill:lightblue
  node005["NOT"]
  style node005 fill:yellow
  node006["z"]
  style node006 fill:lightblue
  node000 -- "L" --> node001
  node000 -- "R" --> node002
  node001 --> node003
  node002 -- "L" --> node004
  node002 -- "R" --> node005
  node005 --> node006
```
