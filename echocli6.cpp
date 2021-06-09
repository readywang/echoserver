#include<sys/socket.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<poll.h>

#define ERR_EXIT(e) do { \
    printf("%s:",e); \
    perror(strerror(errno));\
    exit(EXIT_FAILURE);\
}while(0)

#define BUFSIZE 2048

int main(int argc,char *argv[])
{
    if(argc!=2)
    {
        printf("usage>./echosrv6 port\n");
        exit(-1);
    }
    int port=atoi(argv[1]);

    int confd=0;
    if((confd=socket(AF_INET,SOCK_STREAM,0))<0)
    {
        ERR_EXIT("socket error");
    }
    struct sockaddr_in addr;
    memset(&addr,0,sizeof(addr));
    addr.sin_family=AF_INET;
    addr.sin_port=htons(port);
    inet_aton("127.0.0.1",&addr.sin_addr);
    if(connect(confd,(sockaddr *)&addr,sizeof(addr))<0)
    {
        ERR_EXIT("connect error");
    }
    struct pollfd fds[2];
    fds[0].fd=STDIN_FILENO;
    fds[0].events=POLLIN;
    fds[1].fd=confd;
    fds[1].events=POLLIN;
    char buf[BUFSIZE];
    int nread=0;
    int readeof=0;
    while(true)
    {
        if(poll(fds,2,-1)<0)
        {
            if(errno==EINTR)
            {
                continue;
            }
            ERR_EXIT("poll error");
        }
        for(int i=0;i<2;++i)
        {
            memset(buf,0,sizeof(buf));
            if(fds[i].fd==STDIN_FILENO)
            {
                if((nread=read(STDIN_FILENO,buf,sizeof(buf)))<0)
                {
                    ERR_EXIT("read error");
                }
                else if(nread==0)
                {
                    shutdown(confd,SHUT_WR);
                    readeof=1;
                    continue;
                }
                if(write(confd,buf,nread)!=nread)
                {
                    ERR_EXIT("write error");
                }
            }
            else
            {
                if((nread=read(confd,buf,sizeof(buf)))<0)
                {
                    ERR_EXIT("read error");
                }
                else if(nread==0)
                {
                    if(readeof==1)
                    {
                        continue;
                    }
                    ERR_EXIT("server closed");
                }
                if(write(STDOUT_FILENO,buf,nread)!=nread)
                {
                    ERR_EXIT("write error");
                }
            }
        }
    }
}