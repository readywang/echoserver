#include<sys/epoll.h>
#include<stdio.h>
#include<errno.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<string.h>
#include<sys/types.h>
#include<unistd.h>
#include<fcntl.h>
#include<signal.h>
#include<stdlib.h>

#define EPOLL_EVENT_NUM 1024
#define BUF_SIZE 1024*128
#define ERR_EXIT(e) do{\
    perror(strerror(errno));\
    printf(":%s\n",e);\
    exit(-1);\
}while(0)

/* 信号通知的管道 */
int global_fd[2];

void sig_handler(int sig_num)
{
    write(global_fd[1],&sig_num,sizeof(sig_num));
}

void init_sig_handler()
{
    struct sigaction act;
    memset(&act,0,sizeof(act));
    act.sa_handler=SIG_IGN;
    sigaction(SIGPIPE,&act,NULL);
    sigaction(SIGHUP,&act,NULL);
    act.sa_handler=sig_handler;
    sigaction(SIGINT,&act,NULL);
    sigaction(SIGTERM,&act,NULL);
    sigaction(SIGQUIT,&act,NULL);
}

void set_nonblock(int fd)
{
    int flags=fcntl(fd,F_GETFL,0);
    flags|=O_NONBLOCK;
    fcntl(fd,F_SETFL,flags);
}

void add_fd(int epollfd,int fd,bool ET=true)
{
    epoll_event ev;
    ev.events=EPOLLIN;
    if(ET)
    {
        ev.events|=EPOLLET;
    }
    ev.data.fd=fd;
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&ev);
}

void do_server(int listenfd)
{
    int epollfd=epoll_create(EPOLL_EVENT_NUM);
    if(epollfd<0)
    {
        ERR_EXIT("create epoll fd failed");
    }
    /* 添加监听描述字和管道描述符 */
    add_fd(epollfd,listenfd);
    int ret=pipe(global_fd);
    if(ret<0)
    {
        ERR_EXIT("pipe");
    }
    add_fd(epollfd,global_fd[0],false);
    epoll_event events[EPOLL_EVENT_NUM];
    bool recv_sig=false;
    bool stop=false;
    while(!stop)
    {
        int num=epoll_wait(epollfd,events,EPOLL_EVENT_NUM,-1);
        for(int i=0;i<num;++i)
        {
            if((events[i].events&EPOLLIN)&&(events[i].data.fd==listenfd))
            {
                /* 获取已连接套接字并添加监听 */
                struct sockaddr_in addr;
                socklen_t len=sizeof(addr);
                int confd=accept(listenfd, (struct sockaddr *)&addr,&len);
                if(confd<0)
                {
                    printf("accept error\n");
                    continue;
                }
                set_nonblock(confd);
                add_fd(epollfd,confd);
            }
            else if((events[i].events&EPOLLIN)&&(events[i].data.fd==global_fd[0]))
            {
                /* 信号延后进行处理 */
                recv_sig=true;
            }
            else if(events[i].events&EPOLLIN)
            {
                int sockfd=events[i].data.fd;
                char buf[BUF_SIZE];
                memset(buf,0,sizeof(buf));
                int nread=0;
                while(true)
                {
                    ret=read(sockfd,buf+nread,BUF_SIZE-nread);
                    if(ret<0)
                    {
                        if(errno==EAGAIN)
                        {
                            break;
                        }
                        ERR_EXIT("read error");
                    }
                    else if(ret==0)
                    {
                        /* 客户端关闭连接 */
                        epoll_ctl(epollfd,EPOLL_CTL_DEL,sockfd,NULL);
                        close(sockfd);
                        printf("client closed\n");
                        nread=0;
                        break;
                    }
                    else
                    {
                        nread+=ret;
                    }
                }
                if(nread>0)
                {
                    fputs(buf,stdout);
                    ret=write(sockfd,buf,nread);
                    if(ret!=nread)
                    {
                        ERR_EXIT("write error");
                    }
                }
            }
        }
        if(recv_sig)
        {
            char signals[1024];
            int num=read(global_fd[0],signals,sizeof(signals));
            if(num<=0)
            {
                continue;
            }
            else
            {
                for(int i=0;i<num;++i)
                {
                    switch (signals[i])
                    {
                    case SIGTERM:case SIGINT:case SIGQUIT:
                    {
                        stop=true;
                    }
                    default:
                        break;
                    }
                }
            }
        }
    }
    close(listenfd);
    close(global_fd[0]);
    close(global_fd[1]);
}

int main(int argc,char *argv[])
{
    if(argc<3)
    {
        printf("usage>./echosrv1 -p port\n");
        return -1;
    }
    int ret=0;
    int port=0;
    while((ret=getopt(argc,argv,"p:"))!=-1)
    {
        if(ret=='p')
        {
            port=atoi(optarg);
            break;
        }
        else
        {
            return -1;
        }
    }
    int listenfd=socket(AF_INET,SOCK_STREAM,0);
    if(listenfd<0)
    {
        ERR_EXIT("create listenfd failed");
    }
    /* 设置监听套接字非阻塞 */
    set_nonblock(listenfd);
    /* 绑定ip和端口 */
    sockaddr_in addr;
    memset(&addr,0,sizeof(addr));
    addr.sin_addr.s_addr=htonl(INADDR_ANY);
    addr.sin_port=htons(port);
    addr.sin_family=AF_INET;
    socklen_t len=sizeof(addr);
    ret=bind(listenfd,(struct sockaddr *)&addr,len);
    if(ret<0)
    {
        ERR_EXIT("bind error");
    }
    /* 设置地址重复利用 */
    int val=1;
    setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&val,sizeof(val));
    ret=listen(listenfd,SOMAXCONN);
    if(ret<0)
    {
        ERR_EXIT("listen error");
    }
    do_server(listenfd);
    return 0;
}