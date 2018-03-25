#!/usr/bin/env python
#!encoding: utf-8

import socket
import select
import Queue

listen_fd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
listen_fd.bind(('0.0.0.0', 7777))

listen_fd.listen(2)
print('开始监听...')


# 创建epoll对象, 并注册socket对象的epoll可读事件(EPOLLIN), 相当于托管
epoll = select.epoll()
epoll.register(listen_fd.fileno(), select.EPOLLIN)

conn_fds = {}
msg_queues = {}

while True:
    try:
        ## 主循环, epoll的系统调用, 一旦有网络IO事件发生, poll调用返回. 
        ## 可能同时返回多个事件(比如多个连接同时有数据传入)
        events = epoll.poll(1)
    except KeyboardInterrupt as e:
        print('用户终止了进程...')
        break
    except Exception as e:
        pass
    ## 每个事件对象都包含着发生该事件的描述符, 类似于js中事件回调的event.target
    # $('click', 'div', function(event){
    #   console.log(event.target);
    # });
    for fileno, event in events:
        ## 同样判断如果发生此次事件的是监听套接字, 那一定是有新客户端接入
        ## 向新客户端发送欢迎信息, 然后为其注册可读事件, 顺便创建该连接的消息队列
        if fileno == listen_fd.fileno():
            conn_fd, addr = listen_fd.accept()
            print('收到来自 %s:%d 的连接' % (conn_fd.getpeername()[0], conn_fd.getpeername()[1]))
            conn_fd.send('欢迎!')

            conn_fds[conn_fd.fileno()] = conn_fd
            msg_queues[conn_fd.fileno()] = Queue.Queue()
            ## 貌似不能为同一个描述符创建两个事件监听?
            ## 只能在读完将监听修改为写监听, 然后写完再改成读监听
            epoll.register(conn_fd.fileno(), select.EPOLLIN)
            ## epoll.register(conn_fd.fileno(), select.EPOLLOUT)

        ## 如果是`连接套接字`可读事件
        elif event & select.EPOLLIN:
            data = conn_fds[fileno].recv(1024)
            print('收到来自 %s:%d 的消息: %s' % (conn_fds[fileno].getpeername()[0], conn_fds[fileno].getpeername()[1], data))
            ## 客户端断开后, 要移除事件监听, 移除连接和消息映射
            if data == 'exit':
                print('客户端 %s:%d 断开连接...' % (conn_fds[fileno].getpeername()[0], conn_fds[fileno].getpeername()[1]))

                epoll.unregister(fileno)
                conn_fds[fileno].close()
                del conn_fds[fileno]
                del msg_queues[fileno]
            else:
                epoll.modify(fileno, select.EPOLLOUT)
                msg_queues[fileno].put(data)

        ## 如果是`连接套接字`可写事件
        elif event & select.EPOLLOUT:
            msg_queue = msg_queues[fileno]
            data = msg_queue.get_nowait()
            conn_fds[fileno].send('Hello ' + data)
            epoll.modify(fileno, select.EPOLLIN)

        ## 如果发生EPOLLHUP事件, 则把event对应的socket移除监听队列, 并关闭连接.
        elif event & select.EPOLLHUP:
            epoll.unregister(fileno)
            conn_fds[fileno].close()
            del conn_fds[fileno]
            del msg_queues[fileno]

print('停止监听...')
listen_fd.close()