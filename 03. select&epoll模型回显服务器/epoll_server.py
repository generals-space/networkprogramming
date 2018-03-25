#!/usr/bin/env python
#!encoding: utf-8

import socket
import select

listen_fd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
listen_fd.bind(('0.0.0.0', 7777))

listen_fd.listen(2)
print('开始监听...')


# 创建epoll对象, 并注册socket对象的epoll可读事件(EPOLLIN), 相当于托管
epoll = select.epoll()
epoll.register(listen_fd.fileno(), select.EPOLLIN)

conns = {}
requests = {}
responses = {}

while True:
    ## 主循环, epoll的系统调用, 一旦有网络IO事件发生, poll调用返回. 
    ## 可能同时返回多个事件(比如多个连接同时有数据传入)
    events = epoll.poll(1)

    ## 每个事件对象都包含着发生该事件的描述符, 类似于js中事件回调的event.target
    # $('click', 'div', function(event){
    #   console.log(event.target);
    # });
    for fileno, event in events:
        ## 同样判断如果发生此次事件的是监听套接字, 那一定是有新客户端接入
        if fileno == listen_fd.fileno():
            pass