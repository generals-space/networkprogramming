# C语言-网络编程(一)

## 网络编程基本C/S程序

```c
/*
    server.c
    
    TCP服务端程序, 接受客户端连接后, 可以读取客户端输入字符串, 将其转换为大写后返回, 
    并输出客户端IP与端口, 然后关闭此次连接.
    
    可以配合linux下的nc/telnet工具完成测试.

    参考见<<Linux C语言编程一站式学习>> 37.2.1节, 解释很有用
*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#define MAXLINE 80
#define SERVER_PORT 12345

int main(void)
{
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;

    // listen_fd是监听描述符, 用于bind()
    // connect_fd是连接描述符, 用于read/write()
    int listen_fd, connect_fd;
    char buf[MAXLINE];
    char client_addr_str[INET_ADDRSTRLEN];
    int i, n;

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERVER_PORT);

    bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(listen_fd, 20);
    printf("Accepting connections...\n");

    while(1)
    {
        client_addr_len = sizeof(client_addr);
        connect_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        n = read(connect_fd, buf, MAXLINE);
        // 这里可以得到客户端点分十进制形式的IP, inet_ntop()是一个格式转换函数.
        inet_ntop(AF_INET, &client_addr.sin_addr, client_addr_str, sizeof(client_addr_str));
        printf("received from %s at port %d\n", client_addr_str, ntohs(client_addr.sin_port));

        for(i = 0; i < n; i ++)
            buf[i] = toupper(buf[i]);
        write(connect_fd, buf, n);
        close(connect_fd);
    }
}
```

编译方法

```
$ gcc -c -g server.c
$ gcc -o server server.o
```

解析
------

`connect`与`bind`函数的功能类似, 都是将文件描述符与socket地址对象绑定, 所以它们的参数格式相同. 由于客户端不需要固定的端口号, 因此不必调用`bind()`, 客户端的端口号**由内核自动分配**. 

> 注意, 客户端不是不允许调用`bind()`, 只是没有必要调用`bind()`固定一个端口号, 服务器也不是必须调用`bind()`, 但如果服务器不调用`bind()`, 内核会自动给服务器分配监听端口, 每次启动服务器时端口号都不一样, 客户端要连接服务器就会遇到麻烦.

`client.c`

```c
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#define MAXLINE 80
#define SERVER_PORT 12345

int main(int argc, char *argv[])
{
    struct sockaddr_in server_addr;
    char buf[MAXLINE];
    int sockfd, n;
    char *msg;
    if(argc != 2)
    {
        fputs("usage: ./client message\n", stderr);
        exit;
    }
    msg = argv[1];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
    server_addr.sin_port = htons(SERVER_PORT);

    connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    write(sockfd, msg, strlen(msg));
    n = read(sockfd, buf, MAXLINE);
    printf("Response from server:\n");
    // 直接系统调用, 输出到终端标准输出
    write(STDOUT_FILENO, buf, n);
    printf("\n");
    close(sockfd);
    return 0;
}
```

编译方法

```
$ gcc -c -g client.c
$ gcc -o client client.o
```

C/S程序使用方法:

首先运行`server`程序

```
./server
Accepting connections...
```

然后执行`client`

```
./client abcd
Response from server:
ABCD
```

对应server的输出为

```
./server
Accepting connections...
received from 127.0.0.1 at port 38622
```

## 网络编程IP/端口格式转换实践

参考文章

[inet_aton和inet_network和inet_addr三者比较-《别怕Linux编程》之五](http://roclinux.cn/?p=1160)

网络编程中所用到的IP赋值操作, 需要目标类型是网络字节序的整数, 而不是对人类有友好的点分十进制形式的字符串(如'127.0.0.1'这种形式), 所以在创建socket对象之前, 我们需要手动将点分十进制字符串转换成网络字节的变量.

`inet_network()`, `inet_addr()`, `inet_aton()`这三个都是可选方法.

`inet_addr`和`inet_network`函数都是用于将字符串形式转换为整数形式用的, 两者区别很小, `inet_addr`返回的整数形式是**网络字节序**, 而`inet_network`返回的整数形式是**主机字节序**. 其他地方两者并无二异. 它俩都有一个小缺陷, 那就是当IP是`255.255.255.255`时, 会认为这是个无效的IP地址, 这是历史遗留问题, 其实在目前大部分的路由器上, 这个`255.255.255.255`的IP都是有效的. 

> `inet_network`, 返回的却是主机字节序, 呵呵...

`inet_aton`函数和上面这俩小子的区别就是在于它认为`255.255.255.255`是有效的, 它不会冤枉这个看似特殊的IP地址, 它返回的也是网络字节序的IP. `a` to `n`可以看成是'ASCII' -> 'NETWORK'的类型转换.

```c
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main()
{
    char str[] = "255.255.255.255";
    char str2[] = "255.255.255.254";
    //inet_addr(), inet_network()与inet_aton()的返回值类型, 都是in_addr_t
    in_addr_t net_addr, net_addr2;
    struct sockaddr_in server_addr;

    net_addr = inet_addr(str);
    net_addr2 = inet_addr(str2);
    if(net_addr == -1)
    {
        printf("Error while executing inet_addr() when IP string is %s\n", str);
    }
    else
    {
        printf("net_addr: ip = %lu when IP string is %s\n", ntohl(net_addr), str);
    }
    if(net_addr2 == -1)
    {
        printf("Error while executing inet_addr() when IP string is %s\n", str2);
    }
    else
    {
        printf("net_addr2: ip = %lu when IP string is %s\n", ntohl(net_addr2), str2);
    }
    ////////////////////////////////////////////////////////////////////////////////////
    net_addr = inet_network(str);
    net_addr2 = inet_network(str2);
    if(net_addr == -1)
    {
        printf("Error while executing inet_network() when IP string is %s\n", str);
    }
    else
    {
        printf("net_addr: ip = %lu when IP string is %s\n", ntohl(net_addr), str);
    }
    if(net_addr2 == -1)
    {
        printf("Error while executing inet_network() when IP string is %s\n", str2);
    }
    else
    {
        printf("net_addr2: ip = %lu when IP string is %s\n", ntohl(net_addr2), str2);
    }
    ////////////////////////////////////////////////////////////////////////////////////
    net_addr = inet_aton(str, &server_addr.sin_addr);
    net_addr = inet_aton(str, &server_addr.sin_addr);
    if(net_addr == -1)
    {
        printf("Error while executing inet_aton() when IP string is %s\n", str);
    }
    else
    {
        printf("net_addr: ip = %lu when IP string is %s\n", ntohl(net_addr), str);
        //printf("server_addr.sin_addr");
    }
    if(net_addr2 == -1)
    {
        printf("Error while executing inet_aton() when IP string is %s\n", str2);
    }
    else
    {
        printf("net_addr2: ip = %lu when IP string is %s\n", ntohl(net_addr2), str2);
    }
}
```

数值转换嘛, 不算太难. '255.255.255.254'大概是'11111111 11111111 11111111 11111110', 转换十进制整数为'4294967294', 与上述代码的输出一样.

能从点分十进制转换成网络字节序形式的整数, 就能再转回来, 主要是`inet_ntoa()`这个函数. 使用示例如下

```c
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>

int main()
{
    char *str_addr;
    struct sockaddr_in server_addr;
    server_addr.sin_addr.s_addr = htonl(4294967294);
    str_addr = inet_ntoa(server_addr.sin_addr);
    printf("IP addr is: %s while it's net order is 4294967294\n", str_addr);
}
``` 

你将得到'255.255.255.254', 与上面代码实验中结果相对应.

------

下面两个函数更新一些, 对IPv4地址和IPv6地址都能处理, 而且修正了`inet_addr`的一些缺陷.

- `inet_pton()`: 将点分十进制字符串('127.0.0.1'形式)转换成网络字节序二进制值
- `inet_ntop()`: 和`inet_pton`函数正好相反, inet_ntop函数是将网络字节序二进制值转换成点分十进制串.

下面是`inet_pton()`与`inet_ntop()`函数的示例.

```c
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
int main(int argc, char **argv) 
{
    struct sockaddr_in sockaddr_obj;
    // 一个ip地址变量其实是sockaddr_in结构体中的sin_addr成员变量
    // 它转换成字符串形式所占用的大小是INET_ADDRSTRLEN宏的值, 一般为16
    char *str_addr="172.16.10.196";
    unsigned long net_addr = 0xC40A10AC;
    char str_addr_after[16];
    int result_code;
    
    //INADDR_ANY宏表示inet_addr("0.0.0.0"), 在<netinet/in.h>头文件中有定义
    //服务器端程序绑定了这个, 就相当于绑定了0.0.0.0网卡
    printf("INADDR_ANY: 0x%x\n", INADDR_ANY);
    printf("inet_addr(0.0.0.0)的值为: 0x%x\n", inet_addr("0.0.0.0"));
    printf("INADDR_ANY in sockaddr_in.sin_addr.s_addr: 0x%x\n", htonl(INADDR_ANY));
    //字符串 -> 网络字节
    //转换出来的s_addr是网络字节序的
    result_code = inet_pton(AF_INET, str_addr, &sockaddr_obj.sin_addr);
    printf("%s对应的网络字节序: %2X\n", str_addr, sockaddr_obj.sin_addr.s_addr);

    //网络字节 -> 字符串
    //sizeof(struct sockaddr_in)
    result_code = inet_ntop(AF_INET, &sockaddr_obj.sin_addr, str_addr_after, 16);
    printf("0x%x对应的点分十进制字符串表示: %s\n", net_addr, str_addr_after);
}
```

## 结构体/变量关系梳理

```c
struct sockaddr_in server_addr;
memset(&server_addr, 0, sizeof(server_addr));
server_addr.sin_family = AF_INET;
server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
server_addr.sin_port = htons(SERVER_PORT);

bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
```

关于`sockaddr_in`和`sockaddr`, 前者的`_in`表示Internet, 网络地址结构, 与之对应的还有一种叫做`sockaddr_un`, 即'Unix Domain Socket', 它们两个都可以使用`sockaddr`这个比较通用的地址结构表示. 

所以`sockaddr_in`和`sockaddr`是并列的结构, 指向`sockaddr_in`的结构体的指针也可以指向
sockadd的结构体, 并代替它. 实际上, 在网络编程过程中所用到的函数, 接受的参数大多都是`sockaddr`类型的. 你可以使用sockaddr_in建立你所需要的信息, 在最后进行类型转换就可以了.

然后

- `sin_family`: 指代协议族, 在socket编程中只能是AF_INET
- `sin_port`: 存储端口号(使用网络字节序)
- `sin_addr`: 存储IP地址, 类型是`in_addr`结构体. 它的`s_addr`成员变量用来按照网络字节序存储IP地址.
- `sin_zero`: 为了让`sockaddr`与`sockaddr_in`两个数据结构保持大小相同而保留的空字节. 

## 关于网络字节序与主机字节序的类型转换

一共有4个这样的函数: `htons`, `htonl`, `ntohs`, `ntohl`.

其中n表示network(网络字节序形式), h表示host(主机字节序形式).

s表示short类型, l表示long类型(都是无符号整数).

socket对象的IP/端口的值转换成主机字节序, 可以方便的输出, 注意`inet_aton`/`inet_ntoa`它们接受的参数类型, 是主机字节序还是网络字节序.

另外, 一般`htons`/`ntohs`用于端口转换, `ntohl`/`htonl`用于IP转换...反正我是这么理解的.

当前系统的主机字节序与网络字节序一致时, 它们相当于空函数...貌似.