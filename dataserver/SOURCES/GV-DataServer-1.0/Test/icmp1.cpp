
/*
 * PING program,customize IP packet and ICMP packet
 * 
 * 
 * 
 * 
 * */




#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<netinet/ip_icmp.h>
#include<errno.h>
#include <sys/time.h>

#define BUFFER_SIZE 1024
//对ICMP包的验证
void err_print(char *s)
{
printf("%s",s);
exit(-1);
}
u_short cal_cksum(const u_short*addr,register int len,u_short csum)
{
    register int nleft=len;
    const u_short * w =addr;
    register int sum = csum;
    //1,16位累加
    while(nleft>1)
    {
        sum+= *w++;
        nleft-=2;
    }
    //2,字节长度为奇数，补零成偶数，并再次累加
    if(nleft==1)
    {
        sum+=htons(*(u_char*)w<<8);
    }
    //3，字节长度为偶数
    sum=(sum>>16)+(sum&0xffff);
    sum+=(sum>>16);
    //4，取反，判断
    return ~sum;
}
void dump_pkt(struct ip* ip, struct icmp* icmp)
{
    char src_ip[16];
    char dst_ip[16];
    struct timeval now;
    int rtt;

    gettimeofday(&now, NULL);
    rtt = now.tv_sec - ((struct timeval*)icmp->icmp_data)->tv_sec;

    inet_ntop(AF_INET, &ip->ip_src, src_ip, sizeof(src_ip));
    inet_ntop(AF_INET, &ip->ip_dst, dst_ip, sizeof(dst_ip));
    printf("src ip:%s\tdst_ip%s\t,ttl:%d\trtt:%d\n", src_ip, 
    dst_ip, ip->ip_ttl, rtt);
    return ;
}

bool icmp_parse(char *data)
{
    struct icmp * icmp = NULL;
    struct ip*  ip   = NULL;
    int hlen = 0;
    ip = (struct ip *)data;
    hlen = ip->ip_hl << 2;
    if(ip->ip_p != IPPROTO_ICMP)
    {
        printf("ip proto type no icmp,type:%d\n", ip->ip_p);
        return; 
    }
    icmp = (struct icmp*)(data + hlen);
    if(icmp->icmp_type == ICMP_ECHOREPLY)
    {
        //if(icmp->icmp_id == 0)//id
       // {
            dump_pkt(ip,icmp);
       // }
        //else
        //{
        //    printf("icmp id:%d\n",icmp->icmp_id);
        //}
    }
    else
    {
        printf("icmp type:%d\n",icmp->icmp_type);
    }
    if(icmp->icmp_type==3)
    {
        printf("unreachable\n");
        return false;
    }
    else
    {
        return true;
    }
}

int main(int argc,char*argv[])
{
    
    if(argc != 2){
    err_print("usage: ping <ip-address>\n");
    }
    int fd,tmp;
    struct sockaddr_in sin;
    int buf_size;
    int buf2_size;
    char * buf;
    char * buf2;
    int len;
    struct iphdr *ip_header;
    struct icmphdr *icmp_header;
    fd = socket(PF_INET,SOCK_RAW,IPPROTO_ICMP);
    if(fd<0)
    {
        printf("create socket error\n");
        return -1;
    }
    memset(&sin,0,sizeof(sin));
    sin.sin_port = htons(1111);
    if( (bind(fd,(struct sockaddr*)&sin,sizeof(sin) ))<0)
    {
        printf("bind fail\n");
        close(fd);
        return -1;
    }
    tmp=1;
    setsockopt(fd,0,IP_HDRINCL,&tmp,sizeof(tmp));
    //IP set
    //Ip data=IP head+ICMP head+timestamp
    buf_size = sizeof(struct iphdr)+sizeof(struct icmphdr)+sizeof(struct timeval);
    buf2_size=buf_size;
    buf = (char*)malloc(buf_size);
    buf2 = (char*)malloc(buf2_size);
    //IP head
    memset(buf,0,buf_size);
    memset(buf2,0,buf_size);
    ip_header =(struct iphdr*)buf;
    ip_header->ihl = 5;//ip len
    ip_header->version=4;
    ip_header->tos=0;
    ip_header->tot_len=htons(buf_size);
    ip_header->id=rand();
    ip_header->frag_off=0x40;
    ip_header->ttl=64;
    ip_header->protocol=IPPROTO_ICMP;
    ip_header->check=0;
    ip_header->saddr=0;
    ip_header->daddr=inet_addr(argv[1]);
    //ICMP set
    icmp_header=(struct icmphdr*)(ip_header+1);
    icmp_header->type=ICMP_ECHO;
    icmp_header->code=0;
    icmp_header->un.echo.id=htons(1111);
    icmp_header->un.echo.sequence=0;
    //check
    icmp_header->checksum=
    cal_cksum( (const u_short*)icmp_header,\
    sizeof(struct icmphdr)+sizeof(struct timeval),(u_short)0) ;
    
    memset(&sin,0,sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr= inet_addr(argv[1]);
    int r;
    if(sendto(fd,buf,buf_size,0,( struct sockaddr* )&sin,sizeof(struct sockaddr_in) )<0)
    {
        printf("send error:%s\n",strerror(errno));
        return -1;
    }
    if((r=recvfrom(fd,buf2,buf2_size,0,(struct sockaddr*)&sin,(socklen_t*)&len))<0)
    {      
        printf("receive error\n");
        return -1;
    }
    icmp_parse(buf2);
    return 0;
}
