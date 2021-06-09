#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<string.h>
#include<stdio.h>
#include<errno.h>
#include<unistd.h>
#include<stdlib.h>
#include<signal.h>
#include<sys/wait.h>
#include<sys/un.h>
#include<arpa/inet.h>

#define ERR_EXIT(e) do{ perror(strerror(errno));printf("%s\n",e);exit(-1);}while(0)
#define RECV_BUFFER_SIZE 2048
#define UNIXSTRPATH "./test"

bool isStop=false;

void sig_quit(int iSig)
{
    isStop=true;
}

void sig_child(int iSig)
{
    pid_t pid;
    while((pid=waitpid(-1,NULL,WNOHANG))>0)
    {
        printf("child %d terminated\n",pid);
    }
}

int init_sig_handler()
{
    //注册新号处理函数
    struct sigaction act;
    memset(&act,0,sizeof(act));
    act.sa_handler=sig_quit;
    sigaction(SIGINT,&act,NULL);
    sigaction(SIGTERM,&act,NULL);
    sigaction(SIGQUIT,&act,NULL);
    act.sa_handler=sig_child;
    sigaction(SIGCHLD,&act,NULL);
    return 0;
}

void do_server(int iConfd)
{
    int num=0;
    char pRecvBuff[RECV_BUFFER_SIZE];
    while(true)
    {
        memset(pRecvBuff,0,sizeof(pRecvBuff));
        num=recv(iConfd,pRecvBuff,sizeof(pRecvBuff),0);
        if(num<0)
        {
            if(errno==EINTR)
            {
                continue;
            }
            ERR_EXIT("client closed");
        }
        else if(num ==0)
        {
            ERR_EXIT("client closed");
        }
        else
        {
            printf("%s",pRecvBuff);
            send(iConfd,pRecvBuff,num,0);
        }
    }
}

int main()
{
    int listenfd=socket(AF_LOCAL,SOCK_STREAM,0);
    unlink(UNIXSTRPATH);
    struct sockaddr_un serverAddr;
    memset(&serverAddr,0,sizeof(serverAddr));
    serverAddr.sun_family=AF_LOCAL;
    strcpy(serverAddr.sun_path,UNIXSTRPATH);
    bind(listenfd,(struct sockaddr *)&serverAddr,sizeof(serverAddr));
    listen(listenfd,5);
    int confd;
    struct sockaddr_un cliAddr;
    init_sig_handler();
    while(!isStop)
    {
        memset(&cliAddr,0,sizeof(cliAddr));
        socklen_t len=sizeof(cliAddr);
        if((confd=accept(listenfd,(struct sockaddr *)&cliAddr,&len))<0)
        {
            if(errno==EINTR)
            {
                continue;
            }
            ERR_EXIT("accept error");
        }
        pid_t pid;
        if((pid=fork())<0)
        {
            ERR_EXIT("fork");
        }
        else if(pid==0)
        {
            close(listenfd);
            do_server(confd);
        }
        else
        {
            close(confd);
        }
    }
}