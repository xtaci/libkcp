# libkcp
[![Powered][1]][2] [![Build Status][3]][4]
[1]: https://img.shields.io/badge/KCP-Powered-blue.svg
[2]: https://github.com/skywind3000/kcp
[3]: https://travis-ci.org/xtaci/libkcp.svg?branch=master
[4]: https://travis-ci.org/xtaci/libkcp
kcp client session library for iOS/Android, compatible with [kcp-go](https://github.com/xtaci/kcp-go).    
kcp 客户端库，可用于iOS/Android, 与[kcp-go](https://github.com/xtaci/kcp-go)兼容。        

## Features
1. Optimized for Real-Time Strategy Game.    针对实时游戏优化设计。
2. Stream based interface, you can easily switch from your TCP based protocol to libkcp.    流接口，方便切换现有TCP协议。
3. Less than 300 LOC, just copy ```sess.cpp sess.h``` to your projects and play.    极简设计，拷走即用。

## Usage
libkcp was designed as a Frame-Driven library, the main loop was supposed as:       
libkcp 被设计为帧驱动的库，主循环如：     
```
GameInit()
NetworkInit()
WHILE(NOT GAMEOVER) DO      // e.g:  A 30FPS Game
       Network.Read()
       Network.Write()
       Network.Update()
       Game.Logic()
       Game.Render()
       Wait(33ms)   // frame ticker
END
```

The ```Read/Write/Update``` functions of libkcp are guaranteed to be ***non-blocking***.       
```Read/Write/Update``` 函数保证是```非阻塞```的调用。       

Please read ```main.cpp``` for library usage.        
阅读```main.cpp``` 学习使用方法。      

## Troubleshooting
1. C++11 is required for compiling.    需要C++11进行编译。       
2. Make sure you are using ```Listen()``` function in kcp-go as server listener.   兼容服务器端默认参数，请确保使用标准Listen()函数在服务器端监听。
