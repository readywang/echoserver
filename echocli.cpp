#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<string.h>
#include<stdio.h>
#include<errno.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<stdlib.h>
#define ERR_EXIT(e) do{ perror(strerror(errno));printf("%s\n",e);exit(-1);}while(0)
#define BUFFER_SIZE 2048

void do_client(int iSockfd)
{
    int ret=0;
    char pRecvBuff[BUFFER_SIZE];
    memset(pRecvBuff,0,sizeof(pRecvBuff));
    char pSendBuff[BUFFER_SIZE];
    memset(pSendBuff,0,sizeof(pSendBuff));
    while(fgets(pSendBuff,sizeof(pSendBuff),stdin))
    {
        ret=send(iSockfd,pSendBuff,sizeof(pSendBuff),0);
        if(ret<0)
        {
            if(errno==EINTR)
            {
                continue;
            }
            ERR_EXIT("server closed");
        }
        else if(ret==0)
        {
            ERR_EXIT("server closed");
        }
        ret=recv(iSockfd,pRecvBuff,sizeof(pRecvBuff),0);
        if(ret<0)
        {
            if(errno==EINTR)
            {
                continue;
            }
            ERR_EXIT("server closed");
        }
        else if(ret==0)
        {
            ERR_EXIT("server closed");
        }
        printf("%s\n",pRecvBuff);
        memset(pRecvBuff,0,sizeof(pRecvBuff));
        memset(pSendBuff,0,sizeof(pSendBuff));
    }
}

int main()
{
    int sockfd=socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in serverAddr;
    memset(&serverAddr,0,sizeof(serverAddr));
    serverAddr.sin_port=htons(33333);
    serverAddr.sin_family=AF_INET;
    inet_aton("127.0.0.1",&serverAddr.sin_addr);
    int ret=0;
    if((ret=connect(sockfd,(struct sockaddr*)&serverAddr,sizeof(serverAddr)))<0)
    {
        ERR_EXIT("connect error");
    }
    do_client(sockfd);
}