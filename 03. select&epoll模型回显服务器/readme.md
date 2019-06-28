# select/epoll模型回显服务器

参考文章

1. [python下的select模块使用 以及epoll与select、poll的区别](https://blog.csdn.net/caizs566205/article/details/51193535)
    - select与epoll的区别和联系

相当于开多线程处理客户端连接, 但其实是单线程.

运行方法, 同简单回显的服务器一样, 直接`python select_server.py`或`python epoll_server.py`, 但是之后可以同时启动多个客户端, 服务端都会生成响应. 注意如果不在同一台服务器上运行的话, 记得修改`client.py`中服务端连接的地址.

![](https://gitimg.generals.space/5558f4b6587c06b06272dedcb33b33f1.png)

其实传统的多线程处理方式与select与epoll机制最大的区别在于, 前者是一对一与客户端沟通, 后者则是相当于集中式消息分发. 