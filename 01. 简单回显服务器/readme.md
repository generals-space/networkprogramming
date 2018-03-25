# 一对一 C/S回显服务器

就只是一个server, 一个client, 服务端会把客户端发送的消息回显给客户端.

## 1. 基本流程

1. 服务器启动, 监听指定端口

2. 客户端连接, 服务端发送'欢迎'字样.

3. 客户端发送任何信息, 服务端都会在收到的信息前加上'Hello, '字样并返回.

4. 如果客户端发送'exit', 就关闭本地连接, 同时, 服务端接收到'exit'后也会停止监听.

运行情况如下图

![](https://gitimg.generals.space/bb140dbf012c9143f02aaad5b8236ec3.png)

在这个过程中, 可以看到数据包的如下流向.

![](https://gitimg.generals.space/364d910b8c360bebab63cb89c1b1859a.png)

三次握手就不说了, 三次握手完成后, 服务端会向客户端主动发送`欢迎!`字样. 正是其中的`Data`字段携带的数据, 用python shell打印其中的16进制数据.

```py
>>> print('\xe6\xac\xa2\xe8\xbf\x8e')
欢迎
```
