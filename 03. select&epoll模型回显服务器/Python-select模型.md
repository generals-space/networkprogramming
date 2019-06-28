# Python-select模型

参考文章

1. [基于python select.select模块通信的实例讲解](http://www.jb51.net/article/124202.htm)

2. [Select 模型简介](http://www.jianshu.com/p/edb9ddd51c3d)

`select`函数会在socket连接建立, `send`和`recv`函数调用时返回. 服务端需要一个死循环就一直读取`select`. 就是说, 先select, 再能`accept`, `send`和`recv`之后, 再执行`select`才能发送/接收.

下面的代码是参考文章1中进行改编的, 一个简单的CS程序. 不过参考文章2的更容易理解一点, 至少我在看懂下面代码的70%后再去读参考文章2的示例代码立刻就明白了. 并且参考文章2对select的原理和不足讲述的很清楚, 配合代码看更形象.

`server.py`

```py
#! encoding: utf-8
import socket
import select
import Queue
from time import sleep

## 创建socket
server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.setblocking(0)

# Bind the socket to the port
## 将socket绑定IP与端口
server_address = ('0.0.0.0', 8090)
print ('starting up on %s port %s' % server_address)
server.bind(server_address)

## 开始监听
server.listen(5)

## 我们需要读取信息的socket列表
inputs = [server]

## 我们希望发送响应的socket列表
outputs = []

# Outgoing message queues (socket: Queue)
message_queues = {}

while inputs:
    print ('waiting for the next event')
    try: 
        ## `对inputs`中的服务器端socket进行监听.
        ## `select.select`会阻塞, 一直到有连接建立.
        ## 调用socket的send, recv函数, `select.select`也会有返回.
        readable, writable, exceptional = select.select(inputs, outputs, inputs)
    except select.error, e:
        print(e)
        print('maybe ctrl-c...')
        break

    # 循环判断是否有客户端连接进来, 当有客户端连接进来时select 将触发
    for s in readable:
        ## 判断当前触发的是不是服务端对象, 当触发的对象是服务端对象时, 说明有新客户端连接.
        if s is server:
            ## 当监听socket有返回时表明有连接建立, 服务器端需要accept.
            ## accept后, 我们可以得到名为`client_address`的连接socket(也是socket)
            connection, client_address = s.accept()
            print('connection from', client_address)
            connection.setblocking(0)
            # 将客户端对象也加入到监听的列表中, 当客户端发送消息时select将触发
            inputs.append(connection)
            # 为连接的客户端单独创建一个消息队列，用来保存客户端发送的消息
            message_queues[connection] = Queue.Queue()

        ## 如果不是server的socket(`监听socket`), 那就是连接客户端的socket(`连接socket`)
        else:
            ## 由于客户端连接进来时服务端接收客户端连接请求，将客户端加入到了监听列表中(input_list), 
            ## 客户端发送消息将触发, 所以判断是否是客户端对象触发.
            data = s.recv(1024)

            ## 如果关闭了客户端socket, 那么此时服务器端接收到的data就是''
            if data != '':
                print ('received "%s" from %s' % (data, s.getpeername()))
                # 将收到的消息放入到相对应的socket客户端的消息队列中
                message_queues[s].put(data)
                # 将需要进行回复的`连接socket`放到output 列表中, 让select监听
                if s not in outputs:
                    outputs.append(s)
            else:
                # 客户端断开了连接, 将客户端的监听从input列表中移除
                print ('closing', client_address)
                if s in outputs:
                    outputs.remove(s)
                inputs.remove(s)
                s.close()

                # 移除对应socket客户端对象的消息队列
                del message_queues[s]

    ## 如果现在没有客户端请求, 也没有客户端发送消息时???
    ## 开始对发送消息列表进行处理, 是否需要发送消息
    for s in writable:
        try:
            message_queue = message_queues.get(s)
            send_data = ''
            ## 如果消息队列中有消息,从消息队列中获取要发送的消息
            if message_queue is not None:
                send_data = message_queue.get_nowait()
            else:
                # 客户端连接断开了
                print "has closed "
        except Queue.Empty:
            # 客户端连接断开了
            print "%s" % (s.getpeername())
            outputs.remove(s)
        else:
            # print "sending %s to %s " % (send_data, s.getpeername)
            # print "send something"
            ## 如果取出这条消息之后, 消息队列就空了
            if message_queue is not None:
                s.send(send_data)
            else:
                print "has closed "
            # del message_queues[s]
            # writable.remove(s)
            # print "Client %s disconnected" % (client_address)

    # # Handle "exceptional conditions"
    # 处理异常的情况
    for s in exceptional:
        print ('exception condition on', s.getpeername())
        # Stop listening for input on the connection
        inputs.remove(s)
        if s in outputs:
            outputs.remove(s)
        s.close()

        # Remove message queue
        del message_queues[s]

    ## 实际使用要将sleep移除, 这里可以清楚地看到客户端在发送消息之后1秒左右收到回显.
    sleep(1)
```

这里`select`模型的使用方法可能有点别扭, 所有操作(`accept`, `recv`, `send`)之前/后都要调用`select`. 而且是对3种描述符分别遍历.

`client.py`

```py
#! encoding: utf-8
import socket
import sys
reload(sys)
sys.setdefaultencoding('utf-8')

messages = ['This is the message ', 'It will be sent ', 'in parts ', u'in 中文']
server_address = ('localhost', 8090)

## 同时操作两个socket
socks = [
    socket.socket(socket.AF_INET, socket.SOCK_STREAM), 
    socket.socket(socket.AF_INET, socket.SOCK_STREAM), 
]

print ('connecting to %s port %s' % server_address)

## 连接到服务器
for s in socks:
    s.connect(server_address)

for index, message in enumerate(messages):
    for s in socks:
        print ('%s: sending "%s"' % (s.getsockname(), message + str(index)))
        s.send(bytes(message + str(index)).decode('utf-8'))

for s in socks:
    data = s.recv(1024)
    print ('%s: received "%s"' % (s.getsockname(), data))
    if data != "":
        print ('closingsocket', s.getsockname())
        s.close()
```