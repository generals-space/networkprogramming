# getsockname和getpeername函数

参考文章

1. [UNIX网络编程——getsockname和getpeername函数](http://www.educity.cn/linux/1241293.html)

- `getsockname`一般用于返回与某个套接字关联的本地协议地址.
- `getpeername`一般用于得到与某个套接字关联的对方的地址.

## `getsockname`应用场景

1. 在没有调用`bind`的TCP客户端, `connect`成功返回后, `getsockname`用于返回由内核赋予该连接的本地IP地址和本地端口号. 

2. 在TCP服务端以端口号为0调用bind(此时内核会去选择本地临时端口号)后, `getsockname`用于返回由内核赋予的本地**端口号**(只bind成功就行, 不必listen就能调用). 

3. 在一个以**通配IP**(一般是在多网卡服务器上)地址调用bind的TCP服务端, 与某个客户的连接一旦建立(`accept`成功返回), `getsockname`就可以用于返回由内核赋予该连接的本地IP地址. 在这样的调用中, 套接字描述符参数必须是已连接套接字的描述符, 而不是监听套接字的描述符. 

## `getpeername`应用场景

1. 当一个服务器的是由调用过`accept`的某个进程通过调用exec执行程序时, 它能够获取客户身份的唯一途径便是调用`getpeername`.

`getpeername`只有在连接建立后才调用, 否则不能正确获得对方的地址和端口, 所以它的参数`sockfd`一般是连接描述符而不是监听描述符. 

------

没有连接的UDP程序不能调用`getpeername`, 但是可以调用`getsockname`. 和TCP程序一样, 它的地址和端口不是在调用socket时就指定的, 而是在第一次调用`sendto`函数以后. 

已经连接的UDP, 在调用connect以后, 这2个函数（getsockname, getpeername）都是可以用的. 但是这时意义不大, 因为已经连接（connect）的UDP已经知道对方的地址. 