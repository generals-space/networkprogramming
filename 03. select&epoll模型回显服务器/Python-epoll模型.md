# Python-Epoll模型

参考文章

1. [Epoll 模型简介](http://www.jianshu.com/p/0fb633010296)

2. [python selct模块官方文档epoll节](https://docs.python.org/2/library/select.html#epoll-objects)

3. [我读过最好的Epoll模型讲解](http://blog.csdn.net/mango_song/article/details/42643971)

epoll: edge polling

```py
#!encoding: utf-8
import socket
import select

EOL1 = b'\n\n'
EOL2 = b'\n\r\n'
response = b'HTTP/1.0 200 OK\r\nDate: Mon, 1 Jan 1996 01:01:01 GMT\r\n'
response += b'Content-Type: text/plain\r\nContent-Length: 13\r\n\r\n'
response += b'Hello, world!'

# 创建套接字对象并绑定监听端口
server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
server.bind(('0.0.0.0', 8080))
server.listen(5)
server.setblocking(0)

# 创建epoll对象, 并注册socket对象的epoll可读事件(EPOLLIN)
epoll = select.epoll()
epoll.register(server.fileno(), select.EPOLLIN)

try:
    conns = {}
    requests = {}
    responses = {}
    while True:
        # 主循环, epoll的系统调用, 一旦有网络IO事件发生, poll调用返回. 
        events = epoll.poll(1)
        # 通过事件通知获得监听的文件描述符, 进而处理
        for fileno, event in events:
            ## 如果注册的`监听socket`对象可读, 获取连接, 然后向其注册连接的可读事件
            if fileno == server.fileno():
                conn, address = server.accept()
                conn.setblocking(0)
                ## 对`连接socket`也注册事件
                epoll.register(conn.fileno(), select.EPOLLIN)
                conns[conn.fileno()] = conn
                requests[conn.fileno()] = b''
                responses[conn.fileno()] = response
            ## 如果是`连接socket`可读, 处理客户端发送的信息, 并注册连接对象可写
            elif event & select.EPOLLIN:
                ## 暂存请求头信息, 准备打印到日志
                requests[fileno] += conns[fileno].recv(1024)
                ## 判断是否为http请求(根据请求头的结束标志)
                if EOL1 in requests[fileno] or EOL2 in requests[fileno]:
                    epoll.modify(fileno, select.EPOLLOUT)
                    ## 打印请求头信息
                    print('-' * 40 + '\n' + requests[fileno].decode()[:-2])
            ## 如果是`连接socket`可写, 发送数据到客户端
            elif event & select.EPOLLOUT:
                byteswritten = conns[fileno].send(responses[fileno])
                responses[fileno] = responses[fileno][byteswritten:]
                ## http都是短连接, 响应发送完毕后就停止读写(shutdown)
                if len(responses[fileno]) == 0:
                    epoll.modify(fileno, 0)
                    conns[fileno].shutdown(socket.SHUT_RDWR)
            ## 如果发生EPOLLHUP事件, 则把event对应的socket移除监听队列, 并关闭连接.
            elif event & select.EPOLLHUP:
                epoll.unregister(fileno)
                conns[fileno].close()
                del conns[fileno]
finally:
    epoll.unregister(server.fileno())
    epoll.close()
    server.close()
```

运行这个脚本, 然后curl它, 会得到一个'hello world'的响应. 简单的telnet是不会有结果的, 因为上述代码没有处理纯tcp的请求.