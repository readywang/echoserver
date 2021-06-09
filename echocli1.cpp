#include<sys/socket.h>
#include<stdio.h>
#include<errno.h>
#include<unistd.h>
#include<fcntl.h>
#include<string.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<poll.h>
#include<stdlib.h>
#define BUF_SIZE 1024*128

#define ERR_EXIT(e) do{\
    perror(strerror(errno));\
    printf(":%s\n",e);\
    exit(-1);\
}while(0)

void set_nonblock(int fd)
{
    int flags=fcntl(fd,F_GETFL,0);
    flags|=O_NONBLOCK;
    fcntl(fd,F_SETFL,flags);
}

void do_client(int fd)
{
    /* 设置套接字非阻塞 */
    set_nonblock(fd);
    struct pollfd fds[2];
    fds[0].fd=fd;
    fds[0].events=POLLIN;
    fds[1].fd=STDIN_FILENO;
    fds[1].events=POLLIN;
    while(true)
    {
        int num=poll(fds,2,-1);
        if(num<=0)
        {
            if(errno==EINTR)
            {
                continue;
            }
            ERR_EXIT("poll error");
        }
        for(int i=0;i<2;++i)
        {
            if((fds[i].revents&POLLIN)&&(fds[i].fd==fd))
            {
                char buf[BUF_SIZE];
                memset(buf,0,sizeof(buf));
                int nread=0;
                while(true)
                {
                    int ret=read(fd,buf+nread,BUF_SIZE-nread);
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
                        if(errno!=EAGAIN)
                        {
                        	printf("server closed\n");
			            }
			            return;
                    }
                    else
                    {
                        nread+=ret;
                    }
                }
                printf("%s\n",buf);
            }
            else if((fds[i].revents&POLLIN)&&(fds[i].fd==STDIN_FILENO))
            {
                char buf[BUF_SIZE];
                memset(buf,0,sizeof(buf));
                int num=read(STDIN_FILENO,buf,BUF_SIZE);
                write(fd,buf,num);
            }
        }
    }
}

int main(int argc,char *argv[])
{
    if(argc<5)
    {
        printf("usage>./echocli1 -p port -i IP\n");
        return -1;
    }
    int ret=0;
    int port=0;
    char *ip=NULL;
    while((ret=getopt(argc,argv,"p:i:"))!=-1)
    {
        switch(ret)
        {
            case 'p':
            {
                port=atoi(optarg);
                break;
            }
            case 'i':
            {
                ip=optarg;
                break;
            }
            default:
            {
                return -1;
            }
        }
    }
    int confd=socket(AF_INET,SOCK_STREAM,0);
    if(confd<0)
    {
        ERR_EXIT("create socket failed");
    }
    struct sockaddr_in addr;
    bzero(&addr,sizeof(addr));
    addr.sin_family=AF_INET;
    addr.sin_port=htons(port);
    inet_aton(ip,&(addr.sin_addr));
    socklen_t len=sizeof(addr);
    ret=connect(confd,(struct sockaddr *)&addr,len);
    if(ret<0)
    {
        ERR_EXIT("connect failed");
    }
    do_client(confd);
    return 0;
}
