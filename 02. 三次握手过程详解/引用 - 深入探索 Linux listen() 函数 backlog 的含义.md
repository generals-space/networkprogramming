# 深入探索 Linux listen() 函数 backlog 的含义

原文链接

[深入探索 Linux listen() 函数 backlog 的含义](http://blog.csdn.net/yangbodong22011/article/details/60399728)

## 1. listen()回顾以及问题引入

listen()函数是网络编程中用来使服务器端开始监听端口的系统调用, 首先来回顾下listen()函数的定义：

```c
SYNOPSIS
       #include <sys/types.h>          /* See NOTES */
       #include <sys/socket.h>

       int listen(int sockfd, int backlog);
```

有关于第二个参数含义的问题网上有好几种说法, 我总结了下主要有这么3种：

1. Kernel会为`LISTEN`状态的socket维护一个队列, 其中存放`SYN RECEIVED`和`ESTABLISHED`状态的套接字, backlog就是这个队列的大小. 

2. Kernel会为`LISTEN`状态的socket维护两个队列, 一个是`SYN RECEIVED`状态, 另一个是`ESTABLISHED`状态, 而backlog就是这两个队列的大小之和. 

3. 第三种和第二种模型一样, 但是backlog是队列`ESTABLISHED`的长度.

有关上面说的两个状态`SYN RECEIVED`状态和`ESTABLISHED`状态, 是TCP三次握手过程中的状态转化, 具体可以参考下面的图（在新窗口打开图片）：

![](https://gitimg.generals.space/6710b615ec46e6bcdee1e998531a4791.jpg)

## 2. 正确的解释

那上面三种说法到底哪个是正确的呢? 我下面的说法翻译自这个链接：

[How TCP backlog works in Linux](http://veithen.github.io/2014/01/01/how-tcp-backlog-works-in-linux.html)

当一个应用使用`listen`系统调用让socket进入`LISTEN`状态时, 它需要为该套接字指定一个backlog. backlog通常被描述为**连接队列的限制**. 

由于TCP使用的3次握手, 连接在到达`ESTABLISHED`状态之前经历中间状态`SYN RECEIVED`, 并且可以由`accept`系统调用返回到应用程序. 这意味着TCP/IP堆栈有两个选择来为`LISTEN`状态的套接字实现backlog队列：

> 备注：一种就是两种状态在同一个队列, 一种是分别在独立的队列.

1. 使用**单个队列**实现, 其大小由`listen()`系统调用的backlog参数确定. 当收到`SYN`数据包时, 它发送回`SYN/ACK`数据包, 并将连接添加到队列.  当接收到相应的`ACK`时, 连接将其状态改变为已建立.  这意味着队列可以包含两种不同状态的连接：`SYN RECEIVED`和`ESTABLISHED`.  只有处于后一状态的连接才能通过`accept()`系统调用返回给应用程序. 

2. 使用**两个队列**实现, 一个`SYN`队列（或半连接队列）和一个`accept`队列（或完整的连接队列）. 处于`SYN RECEIVED`状态的连接被添加到`SYN`队列, 并且当它们的状态改变为`ESTABLISHED`时, 即当接收到3次握手中的`ACK`分组时, 将它们移动到`accept`队列.  显而易见, **`accept`系统调用只是简单地从完成队列中取出连接**. 在这种情况下, `listen()`系统调用的backlog参数表示完成队列的大小. 

历史上, BSD派生系统实现的TCP协议使用第一种方法. 该选择意味着当达到最大backlog时, 系统将不再响应于`SYN`分组发送回`SYN/ACK`分组. 通常, TCP的实现将简单地丢弃`SYN`分组, 使得客户端重试. 

在Linux上, 是和上面不同的. 如在listen系统调用的手册中所提到的： 

> 在Linux内核2.2之后, socket backlog参数的形为改变了, **现在它指等待accept的完全建立的套接字的队列长度, 而不是不完全连接请求的数量**. 半连接队列的长度可以使用`/proc/sys/net/ipv4/tcp_max_syn_backlog`设置. 

这意味着当前Linux版本使用上面第二种说法, 有两个队列：具有由系统范围设置指定的大小的SYN队列和应用程序（也就是backlog参数）指定的accept队列. 

OK, 说到这里, 相信backlog含义已经解释的非常清楚了, 下面我们用实验验证下这种说法

## 3. 实验验证

**验证环境**

RedHat 7 
Linux version 3.10.0-514.el7.x86_64

**验证思路**

1. 客户端开多个线程分别创建socket去连接服务端.  

2. 服务端在`listen`之后, 不去调用`accept`, 也就是不会从已完成队列中取走socket连接.  

3. 观察结果, 到底服务端会怎么样? 处于`ESTABLISHED`状态的套接字个数是不是就是backlog参数指定的大小呢? 

我们定义backlog的大小为5:

```c
# define BACKLOG 5
```

看下我系统上默认的SYN队列大小：

![](https://gitimg.generals.space/52b492b26fdd7758681dacda6526d873.jpg)

也就是我现在两个队列的大小分别是 ：

SYN队列大小：256 
ACCEPT队列大小：5

我们的服务端程序`server.c`, 客户端程序`client.c`.

编译运行程序, 并用netstat命令监控服务端8888端口的情况：

```
$ gcc server.c -o server
$ gcc client.c -o client -lpthread -std=c99
```

```
## root执行, 可以看到所有的进程信息 
## watch -n 1 表示每秒显示一次引号中命令的结果 
$ watch -n 1 "netstat -natp | grep 8888" 
```

在两个终端分别执行`server`与`client`.

结果如下：

首先是`watch`的情况：

![](https://gitimg.generals.space/3dc93123ef3fc2691d8ba53dc6731ccb.jpg)

- 因为我们客户端用10个线程去连接服务器, 因此服务器上有10条连接. 

- 第一行的`./server`状态是`LISTEN`, 这是服务器进程(这个不算). 

- 倒数第三行的`./server`是服务器已经执行了一次`accept`. 

- 6条`ESTABLISHED`状态比我们的`BACKLOG`参数5大1. 

- 剩余的`SYN_RECV`状态即使收到了客户端第三次握手回应的`ACK`也不能成为`ESTABLISHED`状态, 因为`BACKLOG`队列中没有位置.

然后过了10s左右, 等到服务器执行了第二个accept之后, 服务器情况如下:

![](https://gitimg.generals.space/520e703f26e08c9247d698ec31f4c063.jpg)

此时watch监控的画面如下:

![](https://gitimg.generals.space/2cc41ad963cb5a76a5f4da4e0eb745c1.jpg)

- 和上面相比, 服务器再次`accept`之后, 多了一条`./server`的连接. 

- 有一条连接从`SYN_RECV`状态转换到了`ESTABLISHED`状态, 原因是`accept`函数从`BACKlOG`完成的队列中取出了一个连接, 接着有空间之后, `SYN`队列的一个链接就可以转换成`ESTABLISHED`状态然后放入`BACKlOG`完成队列了. 

好了, 分析到这里, 有关`BACKLOG`的问题已经解决了, 至于继续上面的实验将backlog的参数调大会怎么样呢? 我试过了, 就是`ESTABLISHED`状态的数量也会增大, 值会是`BACKLOG + 1`, 至于为什么是`BACKLOG + 1`呢? ? ? 我也没有搞懂. 欢迎指教. 

当然, 还有别的有意思的问题是 : 如果ESTABLISHED队列满了, 可是有连接需要从SYN队列转移过来时会发生什么? 

> 一边在喊：让我过来！我满足条件了.  
> 一边淡淡的说：过个毛啊, 没看没地方'住'吗? ～

改天再细说吧, 欢迎评论交流～