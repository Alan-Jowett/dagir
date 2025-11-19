```mermaid
%%{ init: {"theme": "default"} }%%
graph TB
  node000["XOR"]
  style node000 fill:lightpink
  node001["OR"]
  style node001 fill:lightcoral
  node002["AND"]
  style node002 fill:lightgreen
  node003["AND"]
  style node003 fill:lightgreen
  node004["NOT"]
  style node004 fill:yellow
  node005["OR"]
  style node005 fill:lightcoral
  node006["NOT"]
  style node006 fill:yellow
  node007["var1"]
  style node007 fill:lightblue
  node008["var2"]
  style node008 fill:lightblue
  node009["var3"]
  style node009 fill:lightblue
  node010["var4"]
  style node010 fill:lightblue
  node011["var5"]
  style node011 fill:lightblue
  node012["XOR"]
  style node012 fill:lightpink
  node013["var6"]
  style node013 fill:lightblue
  node014["var7"]
  style node014 fill:lightblue
  node000 -- "L" --> node001
  node000 -- "R" --> node002
  node001 -- "L" --> node003
  node001 -- "R" --> node004
  node002 -- "L" --> node005
  node002 -- "R" --> node006
  node003 -- "L" --> node007
  node003 -- "R" --> node008
  node004 --> node009
  node005 -- "L" --> node010
  node005 -- "R" --> node011
  node006 --> node012
  node012 -- "L" --> node013
  node012 -- "R" --> node014
```
