#!/usr/bin/env python
#!encoding: utf-8

import socket
import select
import Queue

listen_fd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
listen_fd.bind(('0.0.0.0', 7777))

listen_fd.listen(2)
print('开始监听...')

inputs, outputs = [listen_fd], []
msg_queues = {}

## 用inputs作循环条件貌似是传统...
## 只要监听套接字还在, 就说明进程不该停止
while inputs:
    try:
        ## `对inputs`中的服务器端socket进行监听.
        ## `select.select`会阻塞, 一直到有连接建立, 或是有数据传入或传出.
        r, w, x = select.select(inputs, outputs, inputs)
    except select.error as e:
        print(e.message)
        break
    except KeyboardInterrupt as e:
        print('用户终止了进程...')
        break
    ## inputs列表中如果有描述符可读, 分两种情况
    ## 一种是有新连接建立, listen_fd可以执行accept()
    ## 一种是有新信息传入, conn_fd可以执行recv()了
    for fd in r:
        if fd is listen_fd:
            conn_fd, addr = fd.accept()
            print('收到来自 %s:%d 的连接' % (conn_fd.getpeername()[0], conn_fd.getpeername()[1]))

            ## 将这个连接套接字加入到监听的列表中
            inputs.append(conn_fd)
            outputs.append(conn_fd)
            conn_fd.send('欢迎!')
            ## 为这个连接套接字建立消息队列
            msg_queues[conn_fd] = Queue.Queue()
        else:
            data = fd.recv(1024)
            print('收到来自 %s:%d 的消息: %s' % (fd.getpeername()[0], fd.getpeername()[1], data))
            ## 这里在接收阶段就处理了
            if data == 'exit':
                print('客户端 %s:%d 断开连接...' % (fd.getpeername()[0], fd.getpeername()[1]))
                del msg_queues[fd]
                inputs.remove(fd)
                outputs.remove(fd)
                fd.close()
            else:
                ## 把收到的消息放到消息队列中, 在下一个对w的循环中决定是否回复
                msg_queues[fd].put(data)

    ## 接下来对消息队列中有消息的描述符进行处理
    for fd in w:
        msg_queue = msg_queues.get(fd)
        if msg_queue is None:
            continue
        try:
            data = msg_queue.get_nowait()
            fd.send('Hello ' + data)
        except Queue.Empty as e:
            pass            

    for fd in x:
        print ('与 %s:%d 的连接发生异常' % (fd.getpeername()[0], fd.getpeername()[1]))
        del msg_queues[fd]
        inputs.remove(fd)
        outputs.remove(fd)
        fd.close()

print('停止监听...')
listen_fd.close()