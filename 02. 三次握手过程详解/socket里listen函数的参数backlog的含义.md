# socket里listen函数的参数backlog的含义

参考文章

1. [Socket里listen函数的参数含意](http://blog.csdn.net/wuhuiran/article/details/1602126)

2. [Socket里listen函数的参数含意](http://m.itboth.com/d/uY7nqe/socket)

3. [深入探索 Linux listen() 函数 backlog 的含义](http://blog.csdn.net/yangbodong22011/article/details/60399728)

4. [TCP SOCKET中backlog参数的用途是什么？](http://www.cnxct.com/something-about-phpfpm-s-backlog/)

5. [关于TCP 半连接队列和全连接队列](http://jm.taobao.org/2017/05/25/525-1/)

6. [Socket `accept queue is full ` 但是一个连接需要从SYN->ACCEPT](https://blog.csdn.net/yangbodong22011/article/details/60468820)

7. [TCP状态转换图](https://gitimg.generals.space/fb4e26fc71e3559a8599b6dcb10d09ce.jpeg)

不管是C还是python或是其他语言, socket编程中都有一个`listen`函数.

```c
int listen(int  sockfd, int  backlog);
```

`listen`接受一个名为`backlog`的参数, 数值类型, 之前一直不明白它有什么含义. 这里记录一下.

参考文章1和2较浅显地指出了, `backlog`是一个队列的长度. 但是超过这个个数的客户端在连接时并不会直接被拒绝而断开连接.

参考文章3则非常客观地评价了网上的几种说法, 并且根据实验验证了自己的猜测. 

参考文章4和5的话题就很深入了, 我们一般接触不到.

这篇笔记只是验证并记录一下参考文章3的推理过程...^_^

## 1. 观点

首先阐明一下观点. 

Linux为每个socket进程维护两个队列, 一般称之为**全连接队列**与**半连接队列**.

全连接队列就是被`listen`的`backlog`参数限制长度的队列, 它们全都是`ESTABLISH`状态的. 在服务端程序开启监听后, 前`backlog + 1`个客户端3次握手后能直接与服务端建立连接. 服务端程序每一次调用`accept`函数其实就是从这个队列中取一个连接进行处理的;

半连接队列就是没能排在前`backlog + 1`的客户端连接队列, 它们是`SYN_RECV`状态, 值得了解的是, 它们其实也与服务端完成了3次握手过程(与服务端握手不如说是与系统内核协议栈握手), 但是被内核挂起了, 所以客户端只能等待, 客户端程序中的`connect`函数没能返回.

~~这个队列的长度也是可控的, 它就是`/proc/sys/net/ipv4/tcp_max_syn_backlog`的值.~~

还有一种就是服务端程序从全连接队列里`accept`了`n`多个连接, 但是一个进程不可能无限制地与服务端建立socket连接, 所以这个`n`也是有限制的, 这个值就是`ulimit -n`, 即打开文件数的值.

## 2. 验证listen的backlog

实验环境: 

- Fedora 24
- python 2.7

我们依然使用之前回显程序的CS代码. `server.py`中`listen`的参数为2, 就是说, 服务端除了有一个`accept`这个正式建立的连接, 最多还可能再开3个`ESTABLISH`状态的连接, 剩下的就只能是`SYN_RECV`的了.

启动服务端程序, 再启动4个客户端程序, 其中第一个客户端与服务端建立了连接, 而另外3个则没有进程处理. `netstat`状态如下.

![](https://gitimg.generals.space/c2c71a7602589545520aed73d290e2b2.png)

由于是在同一台服务器上做的实验, 所以只要看左侧有红色`:7777`的行即可. 排除监听套接字, 还有4行, 其中`127.0.0.1:42112`行后面有`86207/python`, 说明这条连接的处理程序正是我们的主服务端程序, 而另外3行则没有处理程序, 说明它们正在全连接队列中**待命**, 等待被`accept`.

然后再启动一个客户端. 我们将得到第一个`SYN_RECV`状态的连接.

![](https://gitimg.generals.space/fe46823b37a91ff400aacaa3624e8697.png)

同样, 它也是没有被服务端程序处理的, 所以对应行的进程号是空的.

在这次开启这个客户端之前我用wireshark抓包观察了一下, 如下图.

![](https://gitimg.generals.space/596bf1f3deba10b86bdc325a0a3e9397.png)

可以看到, `SYN_RECV`队列中的连接其实也已经完成了3次握手, 但是服务端却在不停地给该客户端发送`[TCP Spurious Retransmission]`包, 导致客户端又得发送3次握手中的最后一个`ACK`包, 于是客户端就被这样挂起了, 实际上客户端的`connect`函数一直没能返回.

> Spurious: 假的, 欺骗性的

## 3. 关于半连接队列的长度

上面提到了, 半连接队列的长度也是有限制的, 这个值就是`/proc/sys/net/ipv4/tcp_max_syn_backlog`的值. 在Fedora 24中, 这个值默认为128.

```
general@localhost ~]$ cat /proc/sys/net/ipv4/tcp_max_syn_backlog
128
```

现在我们同样测试一下. 保持上节的状态不变, 即一个服务端, 5个客户端. 

把`tcp_max_syn_backlog`的值修改为2, 我们准备启动第6, 7个客户端了.

```
[root@localhost ~]# sysctl -a | grep tcp_max_syn
net.ipv4.tcp_max_syn_backlog = 128
[root@localhost ~]# sysctl -w net.ipv4.tcp_max_syn_backlog=2
[root@localhost ~]# sysctl -a | grep tcp_max_syn
sysctl: net.ipv4.tcp_max_syn_backlog = 2
```

这实验有点崩...由于是在同一台服务器上测试, 所以看得可能有点混乱, 这个实验分开测试会比较好.

首先, 处于`SYN_RECV`状态的连接不会持久, 一般过一段时间就会"消失", 无法再在服务端查看到, 而服务端程序本身是完全没有察觉的. 这个时间一般是1分钟, 参考文章6文末也有提过.

但是在客户端, 会认为已经与服务端建立了连接, 虽然`connect`函数并没有完成, 但是用`netstat`查看时会发现已经是`ESTABLISHED`状态了. (而且服务端异常退出时, 全连接队列的客户端会全部挂掉但半连接队列完全没有反应, 相当于它们只是一厢情愿...).

上面的`tcp_max_syn_backlog`参数证明无效, 无论将其值修改为多少, 再同时启动n多个客户端, 服务端上处于`SYN_RECV`的连接最多也只能出现2个. 可以尝试多次修改这个值, 然后查看`SYN_RECV`状态的最大数量, 不会变的.

但是如果将`server.py`的`listen`函数`backlog`设置为4, 那最多就可以看到4个处理`SYN_RECV`的连接.

最终证明半连接队列的长度同样受到`listen`的backlog参数的影响, 修改这个值, 会同时影响全连接和半连接两个队列,  而全连接比半连接个数多一个.