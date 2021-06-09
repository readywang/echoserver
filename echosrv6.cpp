#include<sys/select.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<stdio.h>
#include<errno.h>
#include<stdlib.h>
#include<signal.h>
#include<string.h>
#include<unistd.h>

#define ERR_EXIT(e) do { \
    printf("%s:",e); \
    perror(strerror(errno));\
    exit(EXIT_FAILURE);\
}while(0)

#define LINSEN 9

#define BUFSIZE 2048

bool g_stop=false;

void sig_stop(int sig)
{
    g_stop=true;
}



void init_sighandler()
{
    struct sigaction act;
    act.sa_handler=sig_stop;
    sigemptyset(&act.sa_mask);
    act.sa_flags|=SA_INTERRUPT;
    if(sigaction(SIGTERM,&act,NULL)<0)
    {
        ERR_EXIT("can.t catch SIGTERM");
    }
    if(sigaction(SIGINT,&act,NULL)<0)
    {
        ERR_EXIT("can.t catch SIGINT");
    }
    if(sigaction(SIGQUIT,&act,NULL)<0)
    {
        ERR_EXIT("can.t catch SIGQUIT");
    }
}

int main(int argc,char *argv[])
{
    if(argc!=2)
    {
        printf("usage>./echosrv6 port\n");
        exit(-1);
    }
    int port=atoi(argv[1]);
    int lisenfd=0;
    if((lisenfd=socket(AF_INET,SOCK_STREAM,0))<0)
    {
        ERR_EXIT("create lisenfd failed");
    }
    struct sockaddr_in addr;
    memset(&addr,0,sizeof(addr));
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=htonl(INADDR_ANY);
    addr.sin_port=htons(port);
    if(bind(lisenfd,(sockaddr *)&addr,sizeof(addr))<0)
    {
        ERR_EXIT("bind error");
    }
    if(listen(lisenfd,LINSEN)<0)
    {
        ERR_EXIT("listen error");
    }
    int val=1;
    if(setsockopt(lisenfd,SOL_SOCKET,SO_REUSEADDR,&val,sizeof(val))<0)
    {
        ERR_EXIT("reuse addr error");
    }
    int confds[FD_SETSIZE];
    int i;
    for(i=0;i<FD_SETSIZE;i++)
    {
        confds[i]=-1;
    }
    int maxfd=lisenfd;
    int maxi=-1;
    struct sockaddr_in cliaddr;
    socklen_t len=sizeof(cliaddr);
    fd_set rset,tempset;
    FD_ZERO(&rset);
    FD_SET(lisenfd,&rset);
    int n=0;
    char buf[BUFSIZE];
    init_sighandler();
    while(!g_stop)
    {
        tempset=rset;
        if((n=select(maxfd+1,&tempset,NULL,NULL,NULL))<0)
        {
            if(errno==EINTR)
            {
                continue;
            }
            ERR_EXIT("select error");
        }
        if(FD_ISSET(lisenfd,&tempset))
        {
            int confd=0;
            if((confd=accept(lisenfd,(sockaddr *)&cliaddr,&len))<0)
            {
                ERR_EXIT("accept error");
            }
            printf("recv new connect,client ip is %s,port is %d\n",inet_ntoa(cliaddr.sin_addr),ntohs(cliaddr.sin_port));
            for(i=0;i<FD_SETSIZE;i++)
            {
                if(confds[i]==-1)
                {
                    break;
                }
            }
            if(i==FD_SETSIZE)
            {
                printf("too many clients,close the new client\n");
                close(confd);
            }
            confds[i]=confd;
            if(i>maxi)
            {
                maxi=i;
            }
            if(confd>maxfd)
            {
                maxfd=confd;
            }
            FD_SET(confd,&rset);
            if(--n==0) continue;
        }
        for(i=0;i<=maxi;i++)
        {
            if(FD_ISSET(confds[i],&tempset))
            {
                memset(buf,0,sizeof(buf));
                int nread=0;
                if((nread=read(confds[i],buf,sizeof(buf)))<0)
                {
                    ERR_EXIT("read error");
                }
                else if(nread==0)
                {
                    printf("client closed\n");
                    FD_CLR(confds[i],&rset);
                    close(confds[i]);
                    continue;
                }
                if(write(confds[i],buf,nread)!=nread)
                {
                    ERR_EXIT("write error");
                }
                if(--n==0) break;
            }
        }
    }
    exit(0);
}