#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<string.h>
#include<errno.h>
#include<arpa/inet.h>

#define BUF_SIZE 1024*128
#define ERR_EXIT(e) do { \
    perror(strerror(errno)); \
    printf(":%s\n",e);  \
    exit(-1); \
}while(0)

bool g_stop=false;

void sig_handler(int sig)
{
    g_stop=true;
}

void init_sig_handler()
{
    struct sigaction act;
    act.sa_handler=SIG_IGN;
    act.sa_flags=0;
    sigaction(SIGPIPE,&act,NULL);
    sigaction(SIGHUP,&act,NULL);
    act.sa_handler=sig_handler;
    sigaction(SIGINT,&act,NULL);
    sigaction(SIGTERM,&act,NULL);
    sigaction(SIGQUIT,&act,NULL);
}

void do_server(int fd)
{
    while(!g_stop)
    {
        struct sockaddr_in cli_addr;
        socklen_t len=sizeof(cli_addr);
        char buf[BUF_SIZE]={0};
        int n=recvfrom(fd,buf,sizeof(buf),0,(struct sockaddr *)&cli_addr,&len);
        if(n==-1)
        {
            if(errno==EINTR)
            {
                continue;
            }
            ERR_EXIT("recvfrom error");
        }
        printf("%s\n",buf);
        sendto(fd,buf,n,0,(struct sockaddr *)&cli_addr,len);
    }
}

int main(int argc,char *argv[])
{
    if(argc<3)
    {
        printf("usage>./echosrv2.o -p port\n");
        return -1;
    }
    int ret=0;
    int port=0;
    while((ret=getopt(argc,argv,"p:"))!=-1)
    {
        printf("%s\n",&ret);
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
    init_sig_handler();
    int sockfd=socket(AF_INET,SOCK_DGRAM,0);
    if(sockfd<0)
    {
        ERR_EXIT("create socket failed");
    }
    struct sockaddr_in addr;
    bzero(&addr,0);
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=htonl(INADDR_ANY);
    addr.sin_port=htons(port);
    socklen_t len=sizeof(addr);
    ret=bind(sockfd,(struct sockaddr *)&addr,len);
    if(ret<0)
    {
        ERR_EXIT("bind error\n");
    }
    do_server(sockfd);
    return 0;
}
