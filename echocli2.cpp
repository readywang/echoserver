#include<stdio.h>
#include<string.h>
#include<errno.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>

#define BUF_SIZE 1024*128
#define ERR_EXIT(e) do { \
    perror(strerror(errno)); \
    printf(":%s\n",e);  \
    exit(-1); \
}while(0)


void do_cli(int fd,struct sockaddr *addr,int len)
{
    char buf[BUF_SIZE]={0};
    while(fgets(buf,sizeof(buf),stdin)!=NULL)
    {
        sendto(fd,buf,sizeof(buf),0,(struct sockaddr *)&addr,len);
        struct sockaddr_in srv_addr;
        socklen_t l=sizeof(srv_addr);
        int n=recvfrom(fd,buf,sizeof(buf),0,(struct sockaddr *)&srv_addr,&l);
        if(n==-1)
        {
            if(errno==EINTR)
            {
                continue;
            }
            ERR_EXIT("recvfrom error");
        }
        buf[n]='\0';
        if(l!=sizeof(srv_addr)||(struct sockaddr *)&srv_addr!=addr)
        {
            char str[100]={0};
            inet_ntop(AF_INET,&srv_addr.sin_addr,str,sizeof(str));
            printf("recv data from ip:%s\n",str);
        }
        else
        {
            printf("%s\n",buf);
        }
    }
}

int main(int argc,char *argv[])
{
    if(argc<5)
    {
        printf("usage>./echosrv2.o -p port -i ip\n");
        return -1;
    }
    int ret=0;
    int port=0;
    char *ip=NULL;
    while((ret=getopt(argc,argv,"p:i:"))!=-1)
    {
        if(ret=='p')
        {
            port=atoi(optarg);
        }
        else if(ret=='i')
        {
            ip=optarg;
        }
        else
        {
            printf("error param\n");
            return -1;
        }
    }
    int sockfd=socket(AF_INET,SOCK_DGRAM,0);
    if(sockfd<0)
    {
        ERR_EXIT("create socket failed");
    }
    sockaddr_in addr;
    bzero(&addr,0);
    addr.sin_family=AF_INET;
    addr.sin_port=port;
    inet_pton(AF_INET,ip,&addr.sin_addr);
    do_cli(sockfd,(struct sockaddr *)&addr,sizeof(addr));
    return 0;
}
