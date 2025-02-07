# Protocol

When the client connects to the vtop manager.

All commands are proceeded by \r\n
<- denotes server to client message
-> denotes client to server message

The handshake process begins with:
```
<- 200 vtop 1.0
-> HELO <client>

```
