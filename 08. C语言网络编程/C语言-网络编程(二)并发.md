# C语言网络编程(二)并发

fork: 每accept到一个socket之后，开启一个子进程来负责收发处理工作。

select: 监控文件描述符事件

epoll: 监控文件描述符事件，比select性能优异，可最大支持2W个连接，有死连接时处理能力高

## 1. fork方法

```c
/*
    server.c
*/
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>

#define SERVER_PORT 1234
#define BUF_SIZE 1024

/*
    process_client: 客户端请求处理程序
*/
int process_client(int connect_fd)
{
    char msg[BUF_SIZE];
    int msg_len;
    memset(msg, 0, BUF_SIZE);

    while(1)
    {
        msg_len = read(connect_fd, msg, BUF_SIZE);
        if(msg_len < 0)
        {
            perror("read() error");
            return -1;
        }
        //printf("compare result: %d\n", strncmp(msg, "EOF", 3));
        if(strncmp(msg, "EOF", 3) == 0)
        {
            printf("disconnected...\n");
            close(connect_fd);
            return 0;
        }
        int i;
        for(i = 0; i < msg_len; i ++)
            msg[i] = toupper(msg[i]);
        write(connect_fd, msg, msg_len);
    }

    close(connect_fd);
    return 0;
}

int main()
{
    int listen_fd, connect_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;
    char client_addr_str[INET_ADDRSTRLEN];

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(listen_fd < 0)
    {
        perror("socket() error");
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0)
    {
        perror("bind() error");
        return -1;
    }

    if(listen(listen_fd, 20) != 0)
    {
        perror("listen_fd() error");
        return -1;
    }
    printf("Accepting connections...\n");

    while(1)
    {
        client_addr_len = sizeof(client_addr);
        connect_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if(connect_fd < 0)
        {
            perror("accept() error");
            //这里没有退出, 而是直接进入下一次循环, 接受其他请求.
            continue;
        }
        memset(client_addr_str, 0, sizeof(INET_ADDRSTRLEN));
        inet_ntop(AF_INET, &client_addr.sin_addr, client_addr_str, &client_addr_len);
        printf("connected from %s, socket id: %d\n", client_addr_str, connect_fd);

        int child_pid = fork();
        if(child_pid < 0)
        {
            perror("fork() error");
            break;
        }
        else if(child_pid > 0)
            continue;
        else{
            process_client(connect_fd);
            return 0;
        }
    }
    close(listen_fd);
    return 0;
}
```

上面的代码可以通过在客户端传输"EOF"字符串告知服务器端结束子进程, 但没办法解析到客户端断开事件, 这种情况下子进程无法退出. 解决方法是使用`recv`代替`read`方法. 

示例代码如下

```c
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>

#define SERVER_PORT 1234
#define BUF_SIZE 1024

/*
    process_client: 客户端请求处理程序
*/
int process_client(int connect_fd)
{
    char msg[BUF_SIZE];
    //int msg_len;
    ssize_t msg_len;
    memset(msg, 0, BUF_SIZE);

    while(1)
    {
        msg_len = recv(connect_fd, msg, BUF_SIZE, 0);
        if(msg_len <= 0)
        {
            printf("client disconnect...\n");
            close(connect_fd);
            return -1;
        }
        //printf("compare result: %d\n", strncmp(msg, "EOF", 3));
        if(strncmp(msg, "EOF", 3) == 0)
        {
            printf("disconnected...\n");
            close(connect_fd);
            return 0;
        }
        int i;
        for(i = 0; i < msg_len; i ++)
            msg[i] = toupper(msg[i]);
        write(connect_fd, msg, msg_len);
    }

    close(connect_fd);
    return 0;
}
...
```

处理子进程退出事件, 添加到main()函数的while循环之间.

```c
void handle_sigcld(int signo)
{
    int pid,status;
    pid = waitpid( -1, &status, 0);//-1表示等待任何子进程
    //printf("child process %d exit with %d\n",pid,status);
}
```