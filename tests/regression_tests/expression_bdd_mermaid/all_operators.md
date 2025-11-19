```mermaid
%%{ init: {"theme": "default"} }%%
graph TB
  node000("var1")
  node001("var2")
  node002("var3")
  node003("var4")
  node004("var4")
  node005("var5")
  node006("var5")
  node007("var6")
  node008("var6")
  node009("var7")
  node010("var7")
  node011["0"]
  style node011 fill:lightgray
  node012["1"]
  style node012 fill:lightgray
  node000 --> node001
  node000 --> node002
  node001 --> node002
  node001 --> node003
  node002 --> node003
  node002 --> node004
  node003 --> node005
  node003 --> node007
  node004 --> node006
  node004 --> node008
  node005 --> node007
  node005 --> node012
  node006 --> node008
  node006 --> node011
  node007 --> node009
  node007 --> node010
  node008 --> node009
  node008 --> node010
  node009 --> node011
  node009 --> node012
  node010 --> node011
  node010 --> node012
```
