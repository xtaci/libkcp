# libkcp
[![Powered][1]][2]
[1]: https://img.shields.io/badge/KCP-Powered-blue.svg
[2]: https://github.com/skywind3000/kcp
kcp client session library for iOS/Android, compatible with [kcp-go](https://github.com/xtaci/kcp-go).

## Features
1. Optimized for Real-Time Strategy Game.
2. No Forward Error Correction(FEC).
3. No Packet Level Encryption.
4. Abstraction on Stream(```ReadWriter``` interface).
5. Less than 300 LOC

## Troubleshooting
1. C++11 is required for compiling.
2. Make sure you are using ```Listen()``` function in kcp-go.
