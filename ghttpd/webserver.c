#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

#define SERVER_STR "Server: shttpd/1.0.0\r\n"
#define SERVER_PORT 1234

void request_handler(int);
int get_line(int, char *, int);

/*
    @Function: 501状态, 不受支持的请求方法.
*/
void status_501(int connect_fd)
{
    char buf[1024];
    sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
    send(connect_fd, buf, sizeof(buf), 0);
}

/*
    @Function: 404错误, 文件不存在
*/
void status_404(int connect_fd)
{
    char buf[1024];
    sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
    send(connect_fd, buf, sizeof(buf), 0);
}

void status_400(int connect_fd)
{
    char buf[1024];
    sprintf(buf, "HTTP/1.0 400 BAD REQUEST\r\n");
    send(connect_fd, buf, sizeof(buf), 0);
}

void status_500(int connect_fd)
{
    char buf[1024];
    sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n");
    send(connect_fd, buf, sizeof(buf), 0);
}

/*
    从目标socket套接字中获取一行.
    http请求的换行符可能有3种: 换行符linefeed(\n), 回车符carriage return(\r), 或是两者的结合(\r\n)

    如果直接buf装满了也还没发两现换行符则把buf的最后一个字符设置为'\0';
    如果读到上面3种任意一种换行符, 则buf的结尾为'\n', 结尾后再跟一个'\0'.
*/
int get_line(int connect_fd, char *buf, int size)
{
    int i = 0;
    char c = '\0';
    int n;
    // 如果c为字符'\n', 说明这一行已经读取完毕(当然, 是在buf未满的情况下)
    while((i < size -1) && (c != '\n'))
    {
    	n = recv(connect_fd, &c, 1, 0);
	if(n <= 0)
	{
	    c = '\n';
	}
	else
	{
	    // 如果发现接收到的是字符'\r', 则继续, 因为下一个很可能是'\n', 它们共同组成http协议中的换行
	    if(c == '\r')
	    {
	    	// MSG_PEEK标志可以使用下一次recv读取时依然从此次内容开始, 可以理解为窗口不滑动
	        n = recv(connect_fd, &c, 1, MSG_PEEK);
            // 如果的确是换行符就把它吸收到, 窗口前移, 不能影响下一次读取.
            if((n > 0) && (c == '\n'))
            {
                recv(connect_fd, &c, 1, 0);
            }
            // 如果读到了其他字符, 说明'\r'本身就是一个换行符了, 结束读取
            else
            {
                c = '\n';
            }
	    }
	    buf[i] = c;
	    i ++;
	}
    }
    buf[i] = '\0';
    return i;
}


/*
    确切的说这是一个200响应的响应头
*/
void response_header(int connect_fd)
{
    char buf[1024];
    strcpy(buf, "HTTP/1.0 200 OK\r\n");
    send(connect_fd, buf, strlen(buf), 0);
    strcpy(buf, SERVER_STR);
    send(connect_fd, buf, strlen(buf), 0);
    strcpy(buf, "Content-type: text/html\r\n");
    send(connect_fd, buf, strlen(buf), 0);
    strcpy(buf, "\r\n");
    send(connect_fd, buf, strlen(buf), 0);
}

void response_file(int connect_fd, FILE *file)
{
    char buf[1024];
    int msg_len;
    // fgets以换行为分隔符.
    fgets(buf, sizeof(buf), file);
    while(!feof(file))
    {
        msg_len = send(connect_fd, buf, strlen(buf), 0);
        // 这一句只有下次http请求到来时才会输出, 应该是下次请求到来前socket不算真正的关闭??? 
        printf("send %d bytes", msg_len);
        fgets(buf, sizeof(buf), file);
    }
}
void serve_file(int connect_fd, const char *filename)
{
    FILE *file = NULL;
    char buf[1024];
    int buf_len = 1;

    buf[0] = 'A'; buf[1] = '\0';
    while(buf_len > 0 && strcmp("\n", buf))
        buf_len = get_line(connect_fd, buf, sizeof(buf));
    
    file = fopen(filename, "r");
    if(file == NULL)
        status_404(connect_fd);
    else
    {
        response_header(connect_fd);
        response_file(connect_fd, file);
    }
    fclose(file);
}

void exec_cgi(int connect_fd, const char *filename, const char *method, const char *query_str)
{
    char buf[1024];
    int buf_len = 1;
    int cgi_input[2];
    int cgi_output[2];
    pid_t pid;
    int status;
    int i;
    char c;
    int cnt_len = -1;

    buf[0] = 'A'; buf[1] = '\0';
    if(strcasecmp(method, "GET") == 0)
    {
        while(buf_len > 0 && strcmp("\n", buf))
            buf_len = get_line(connect_fd, buf, sizeof(buf));
    }
    else
    {
        buf_len = get_line(connect_fd, buf, sizeof(buf));
        while(buf_len > 0 && strcmp("\n", buf))
        {
            // buf[15]正好是Content-Length:的冒号之后的位置.
            buf[15] = '\0';
            if(strcasecmp(buf, "Content-Length:") == 0)
            {
                cnt_len = atoi(&buf[16]);
            }
            buf_len = get_line(connect_fd, buf, sizeof(buf));
        }
        // 没有找到content-length字段
        if(cnt_len == -1){
            status_400(connect_fd);
            return ;
        }
    }

    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    send(connect_fd, buf, strlen(buf), 0);

    /* 建立管道*/
    if (pipe(cgi_output) < 0) {
        /*错误处理*/
        status_500(connect_fd);
        return ;
    }
    /*建立管道*/
    if (pipe(cgi_input) < 0) {
        /*错误处理*/
        status_500(connect_fd);
        return ;
    }
    if ((pid = fork()) < 0 ) {
        /*错误处理*/
        status_500(connect_fd);
        return ;
    }

    if(pid == 0)
    {
        // 由于管道的存在, 子进程中无法通过标准输出打印到控制台任何信息,
        // 需要在父进程的read方法中取到.
        char env_method[255];
        char env_query[255];
        char env_length[255];

        dup2(cgi_input[0], 0);
        dup2(cgi_output[1], 1);
        close(cgi_input[1]);
        close(cgi_output[0]);

        // 设置环境变量
        sprintf(env_method, "REQUEST_METHOD=%s", method);
        putenv(env_method);

        if (strcasecmp(method, "GET") == 0) {
            /*设置 query_string 的环境变量*/
            sprintf(env_query, "QUERY_STRING=%s", query_str);
            putenv(env_query);
        }
        else 
        {
            /*设置 content_length 的环境变量*/
            sprintf(env_length, "CONTENT_LENGTH=%d", cnt_len);
            putenv(env_length);
        }
        printf("before exec %s...\n", filename);
        //使用execl函数运行cgi
        execl(filename, filename, NULL);
        exit(0);
    }
    else
    {
        close(cgi_input[0]);
        close(cgi_output[1]);
        // 把POST请求中带的数据, 传递给CGI子进程.
        if(strcasecmp(method, "POST") == 0)
        {
            for(i = 0; i < cnt_len; i ++)
            {
                recv(connect_fd, &c, 1, 0);
                write(cgi_input[1], &c, 1);
            }
        }
        while(read(cgi_output[0], &c, 1) > 0)
        {
            printf(&c);
            send(connect_fd, &c, 1, 0);
        }
        close(cgi_input[1]);
        close(cgi_output[0]);
        waitpid(pid, &status, 0);
    }
}

void request_handler(int connect_fd)
{
    char buf[1024];
    int buf_len;
    char method[255];
    char url[255];
    char path[512];
    int i, j;
    struct stat filestat;
    int is_cgi = 0;
    char *query_str = NULL;

    // 得到http请求的第一行, 一般是: GET / http1.1\r\n这种形式
    buf_len = get_line(connect_fd, buf, sizeof(buf));
    printf("The first line in request is %s\n", buf);

    // 从第一行中取出请求方法GET或是POST
    i = 0;
    while(!isspace(buf[i]) && i < (buf_len - 1))
    {
        method[i] = buf[i];
        i ++;
    }
    method[i] = '\0';
    printf("The method is %s\n", method);

    // strcasecmp()忽略大小写比较字符串
    if(strcasecmp(method, "GET") && strcasecmp(method, "POST"))
    {
        status_501(connect_fd);
        return ;
    }

    // POST请求的时候开启CGI
    if(strcasecmp(method, "POST") == 0)
    {
        is_cgi = 1;
    }

    // 接下来读取请求中的url, 还是在第一行.
    while(isspace(buf[i]) && i < (buf_len - 1))
    {
        i ++;
    }
    j = 0;
    while(!isspace(buf[i]) && i < (buf_len - 1) && j < (sizeof(url) - 1))
    {
        url[j] = buf[i];
        i ++; j ++;
    }
    url[j] = '\0';
    printf("The url is %s\n", url);

    // 处理得到url中的请求参数, 格式为?a=1&b=2&c=3
    if(strcasecmp(method, "GET") == 0)
    {
        query_str = url;
        while(*query_str != '?' && *query_str != '\0')
        {
            // 当前字符不是?, 也没有到字符串末尾, 则指针后移
            query_str ++;
        }
        // 如果*query_str是'?'(由于上面while的存在, 如果不是'?'就没什么事了...)
        if(*query_str == '?')
        {
            is_cgi = 1;
            *query_str = '\0';
            query_str ++;
            // 这样舍弃前面的字符, 会不会出问题啊??
            // ...错了, query_str只是一个字符指针, 本身并没有存储数据
            // *query_str写入'\0'时, 其实是在url空间中修改的, 这样url只能输出'?'符号前的数据
            // 而query_str则可以得到'?'符号后面的数据.
            printf("query string is %s\n", query_str);
        }
    }

    // 组装文件路径, 默认在htdocs目录中寻找
    sprintf(path, "htdocs%s", url);
    if(path[strlen(path) - 1] == '/')
    {
        strcat(path, "index.html");
    }
    printf("Target path is %s\n", path);

    // 最终路径必须定位到一个文件
    if(stat(path, &filestat) == -1)
    {
        // 如果定位不到目标文件, 说明文件找不到, 请求头的剩余信息也没什么用了, 直接丢弃吧
        while(buf_len > 0 && strcmp("\n", buf))
            buf_len = get_line(connect_fd, buf, sizeof(buf));
        status_404(connect_fd);
    }
    else
    {
        // tinyhttpd中根据文件是否具有可执行权限判断是否执行cgi, 这是不合理的.
        // if((filestat.st_mode & S_IXUSR) || (filestat.st_mode & S_IXGRP) || (filestat.st_mode & S_IXOTH))
        //     is_cgi = 1;
        
        if(!is_cgi){
            serve_file(connect_fd, path);
        }
        else
        {
            exec_cgi(connect_fd, path, method, query_str);
        }
    }
    close(connect_fd);
}

int tcp_server()
{
    struct sockaddr_in server_addr;
    int listen_fd;

    if((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket create error");
        exit(1);
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERVER_PORT);

    if(bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
    	perror("bind error");
	    exit(1);
    }

    if(listen(listen_fd, 20) < 0)
    {
    	perror("listen error");
	    exit(1);
    }
    printf("accepting connections on port %d\n", SERVER_PORT);
    return listen_fd;
}

int main()
{
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;
    int listen_fd, connect_fd;
    pthread_t subthread;

    listen_fd = tcp_server();

    while(1)
    {
    	client_addr_len = sizeof(client_addr);
        connect_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_addr_len);
    	if(connect_fd < 0)
        {
            perror("accept error");
            exit(1);
        }

        if(pthread_create(&subthread, NULL, request_handler, connect_fd) != 0)
        {
            perror("thread create error");
        }
    }
    close(listen_fd);
    return 0;
}
