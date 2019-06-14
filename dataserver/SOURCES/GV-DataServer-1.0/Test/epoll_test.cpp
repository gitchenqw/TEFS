#include <sys/epoll.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>         
#include <sys/socket.h>
#include <unistd.h>
#define EPOLL_MAXSIZE 1024
static uint32_t  epoll_fd;                      //fd by epoll_create
static uint32_t  wait_num;                      //num of detec
class A
{
   public:
     virtual void fun(int ){printf("A class\n");}
};
class B:public A
{
 public:
   virtual void fun(int eveb );
};
void B::fun(int)
{
    printf("B class\n");
}
int main()
{
    if((epoll_fd = epoll_create(EPOLL_MAXSIZE)) < 0)
        printf("create epoll failed\n");
    epoll_event req;
    req.data.fd = 0;
    req.events =EPOLLIN;
    if(epoll_ctl(epoll_fd,EPOLL_CTL_ADD,0,&req)<0)
   {
         printf("ADD epoll: failed,%s\n",strerror(errno));
    }
    req.events =EPOLLOUT; 
    if(epoll_ctl(epoll_fd,EPOLL_CTL_MOD,0, &req)<0)
         printf("mod epoll: failed%s\n",strerror(errno)); 
    //::close(0);
    if(epoll_ctl(epoll_fd,EPOLL_CTL_DEL,0,0) <0)
        printf("1delete epoll: failed\n");
    
    return 0;
}




