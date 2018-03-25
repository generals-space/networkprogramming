#!/usr/bin/env python
#!encoding: utf-8

import socket

listen_fd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
listen_fd.bind(('0.0.0.0', 7777))

listen_fd.listen(2)
print('开始监听...')

## accept是一个阻塞方法, 如果没有客户端连接进入就停在这里
## addr是一个元组, 格式为 ('客户端IP', 客户端端口)
conn_fd, addr = listen_fd.accept()
print('收到来自 %s 的连接' % addr[0])
conn_fd.send('欢迎!')

## 循环接受客户端发送的消息
while True:
    data = conn_fd.recv(1024)
    print(data)
    if data == 'exit':
        print('客户端 %s 断开连接...' % addr[0])
        conn_fd.close()
        break
    else:
        conn_fd.send('Hello, ' + data)

listen_fd.close()
print('停止监听...')