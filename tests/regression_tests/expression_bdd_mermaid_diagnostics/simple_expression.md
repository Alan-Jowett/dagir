```mermaid
%%{ init: {"theme": "default"} }%%
%% classDef error fill:#ffdce0,stroke:#d93025,stroke-width:2px
%% classDef critical fill:#fff4e5,stroke:#f29900,stroke-width:2px
graph TB
  node000("a")
  node001("b")
  node002("c")
  node003("d")
  node004["0"]
  style node004 fill:lightgray
  node005["1"]
  style node005 fill:lightgray
  node000 --> node001
  node000 --> node002
  node001 --> node002
  node001 --> node005
  node002 --> node003
  node002 --> node004
  node003 --> node004
  node003 --> node005
```
