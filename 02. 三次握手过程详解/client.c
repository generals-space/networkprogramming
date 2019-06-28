#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<string.h>
#include<strings.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>

#define PORT 8888
#define thread_num 10  //定义创建的线程数量

struct sockaddr_in serv_addr;

void *func() 
{
    int conn_fd;
    conn_fd = socket(AF_INET,SOCK_STREAM,0);
    printf("conn_fd : %d\n",conn_fd);

    if( connect(conn_fd,(struct sockaddr *)&serv_addr,sizeof(struct sockaddr_in)) == -1) {
        printf("connect error\n");
    }

    while(1) {}
}

int main(int argc,char *argv[])
{
    memset(&serv_addr,0,sizeof(struct sockaddr_in));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_aton("192.168.30.155", (struct in_addr *)&serv_addr.sin_addr); //此IP是局域网中的另一台主机
    int retval;

    //创建线程并且等待线程完成
    pthread_t pid[thread_num];
    for(int i = 0 ; i < thread_num; ++i)
    {
        pthread_create(&pid[i],NULL,&func,NULL);
    }

    for(int i = 0 ; i < thread_num; ++i)
    {
        pthread_join(pid[i],(void*)&retval);
    }

    return 0;
}