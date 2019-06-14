
/*
 * no customize IP header
 * 
 * */
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>

int rawfd;
int id;
int seq;
struct sockaddr_in addr;

void err_print(char *s)
{
printf("%s",s);
exit(-1);
}

unsigned short  in_cksum(uint16_t * addr, int len)
{
int	nleft = len;
uint32_t	sum = 0;
uint16_t	*w = addr;
uint16_t	answer = 0;

/*
* Our algorithm is simple, using a 32 bit accumulator (sum), we add
* sequential 16 bit words to it, and at the end, fold back all the
* carry bits from the top 16 bits into the lower 16 bits.
*/
while (nleft > 1)  {
sum += *w++;
nleft -= 2;
}

/* 4mop up an odd byte, if necessary */
if (nleft == 1) {
*(unsigned char *)(&answer) = *(unsigned char *)w ;
sum += answer;
}

/* 4add back carry outs from top 16 bits to low 16 bits */
sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
sum += (sum >> 16);	/* add carry */
answer = ~sum;	/* truncate to 16 bits */
return(answer);
}

void ping(int signo)
{
struct icmp* icmp = NULL;
static char buf[1024];

memset(buf, 0, sizeof(buf));

icmp = (struct icmp*)buf;
icmp->icmp_type = ICMP_ECHO;
icmp->icmp_code = 0;
icmp->icmp_id	= id;
icmp->icmp_seq = seq++;

gettimeofday((struct timeval*)icmp->icmp_data, NULL);

icmp->icmp_cksum = in_cksum((uint16_t *)icmp, sizeof(struct icmp));

sendto(rawfd, icmp, sizeof(struct icmp), 0, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
alarm(1);
return ;
}

void dump_pkt(struct ip* ip, struct icmp* icmp)
{
char src_ip[16];
char dst_ip[16];
struct timeval now;
struct timeval* past = (struct timeval*)icmp->icmp_data;
int rtt;

gettimeofday(&now, NULL);
rtt = now.tv_sec*1000000+now.tv_usec - past->tv_sec*1000000-past->tv_usec;

inet_ntop(AF_INET, &ip->ip_src, src_ip, sizeof(src_ip));
inet_ntop(AF_INET, &ip->ip_dst, dst_ip, sizeof(dst_ip));
printf("src ip:%s\tdst_ip%s\t,ttl:%d\trtt:%f (ms)\n", src_ip, 
dst_ip, ip->ip_ttl, (double)rtt/1000);
return ;
}

void icmp_parse(char *data)
{
struct icmp * icmp = NULL;
struct ip*  ip   = NULL;
int hlen = 0;

ip = (struct ip *)data;

hlen = ip->ip_hl << 2;
if(ip->ip_p != IPPROTO_ICMP){
printf("ip proto type no icmp,type:%d\n", ip->ip_p);
return; 
}
icmp = (struct icmp*)(data + hlen);

if(icmp->icmp_type == ICMP_ECHOREPLY)
{
    if(icmp->icmp_id == id)
    {
        dump_pkt(ip,icmp);
    }
    else
    {
        printf("icmp id:%d\n",icmp->icmp_id);
    }
}
else
{
        printf("icmp type:%d\n",icmp->icmp_type);
}

}

int main(int argc , char **argv)
{
int rc;
int len;
char recv[1024];

if(argc != 2){
err_print("usage: ping <ip-address>\n");
}

rc = inet_pton(AF_INET, argv[1], &addr.sin_addr);
if(rc != 1){  //!!!!
err_print("ip address not right\n");
}
addr.sin_family = AF_INET;

if(getuid() != 0){
err_print("must be root\n");
}

rawfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
if(rawfd < 0){
err_print("create socket error\n");
}

id = getpid() & 0xFFFF;

signal(SIGALRM, ping);
alarm(1);

while(1)
{
len = read(rawfd, recv, sizeof(recv));
if(len < 0){
err_print("read error\n");
}

if(len == 0){
continue;
}

/*parse icmp hdr*/
icmp_parse(recv);
}
}
