
/*
 * (C) yus 2013-2014

 * Usage:
 *    --help
 *
 * 
 */
#include "OS.h"
#include "DSServerTest.h"
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <sys/time.h>
       
/* values */
volatile int timerexpired=0;
bool loop = false;
int TCP = 1;
int UDP = 1;
char* method;
int32_t success=0;
int32_t failed=0;
int     ReadError = 0;
uint64_t bytes=0;
int32_t totaltry=0;
/* globals */
#define PROGRAM_VERSION "version 1.5"
int debuglevel = 0;
int clients=1;
int force=0;
int force_reload=0;
int port=DEFAULTPORT;
char *host = "127.0.0.1";
int m_type = UNKNOWN;
int runtime=0;
UInt64 begin_time;
UInt64 end_time;
double real_runtime;
/* internal */
int mypipe[2];
int processnum = 1;
int threadnum = 100;
int threadalive = threadnum;
int totalthread = 0;
SInt64 process_time = 0; //ms
OSCond fCond;
OSMutex mutex;
char* inputfile = NULL;
char* outfile = NULL;
//
char* backhost1 = "10.0.3.181";
int   backport1 = 5800;
char* backhost2 = "10.0.3.182";
int   backport2 = 5800;
char* backhost3 = "10.0.3.183";
int   backport3 = 5800;


int64_t fileid = 1;
int32_t chunkid = 1;

static const struct option long_options[]=
{
 {"TCP",no_argument,NULL,'0'},
 {"UDP",no_argument,NULL,'1'},
 {"time",required_argument,NULL,'t'},
 {"loop",no_argument,NULL,'l'},
 {"help",no_argument,NULL,'?'},
 {"version",no_argument,NULL,'V'},
 {"verbose",required_argument,NULL,'v'},
 {"remoteadress",required_argument,NULL,'p'},
 {"message",required_argument,NULL,'m'},
 {"clients",required_argument,NULL,'c'},
 {NULL,0,NULL,0}
};

/* prototypes */
static int test(void);
static void* pthread_testcore(void*);
static void testcore(const char *host,const int port,int type);

static void alarm_handler(int signal)
{
    timerexpired=1;
}	

static void int_handler(int signal)
{
    timerexpired=1;
}

static void usage(void)
{
   fprintf(stderr,
	"ServerTest [option]... \n"
    "  -0|--TCP                         Run using TCP.\n"
    "  -1|--UDP                         Run using UDP.\n"
	"  -t|--time <sec>                  Run test for <sec> seconds. Default 5.\n"
    "  -l|--loop                        if try again in <sec> seconds.\n"
	"  -p|--remoteaddress<server:port>  remote server address.\n"
	"  -c|--clients <n>                 Run <n> clients at once. Default one.\n"
    "  -m|--message                     customized message type\n"
    "  -f|--file                        file need sending or writing"
    "  -v|--verbose                     debug level"
	"  -?|-h|--help                     Display this information.\n"
	"  -V|--version                     Display program version.\n"   
    "Message Types:\n"
"   0:CLINET_PUT,   1:CLIENT_GET,   2:NS_DELETE.\n"
"   3:NS_BACKUP,    4:B_SERVER_STATUS\n"
"   Others:UNKNOWN\n"
	);
}

int main(int argc, char *argv[])
{
    OS::Initialize();
    int opt=0;
    int options_index=0;
    char *tmp=NULL;

    while((opt=getopt_long(argc,argv,"01Vv:t:lp:c:m:f:?h",long_options,&options_index))!=EOF )
    {
    switch(opt)
    {
        case  '0' : 
               TCP = 1,UDP = 0;break;
                
        case  '1' :
                TCP = 0,UDP = 1;break;
        case  'l' :
                loop=true;break;
        case 'V': printf(PROGRAM_VERSION"\n");exit(0);
        case 'v': debuglevel = atoi(optarg); break;//
        case 't': runtime=atoi(optarg);break;
        case 'f': inputfile = optarg;outfile = optarg;break;
        case 'p': 
            /* proxy server parsing server:port */
            tmp=strrchr(optarg,':');
            host=optarg;
            if(tmp==NULL)
            {
                usage();
                return 2;
            }
            if(tmp==optarg)
            {
                fprintf(stderr,"Error in option --p %s: Missing hostname.\n",optarg);
                return 2;
            }
            if(tmp==optarg+strlen(optarg)-1)
            {
                fprintf(stderr,"Error in option --p %s Port number is missing.\n",optarg);
                return 2;
            }
            *tmp='\0';
            port=atoi(tmp+1);break;
            case 'm':
                m_type=atoi(optarg);break;
            case ':':
            case 'h':
            case '?': usage();return 2;
            case 'c': clients=atoi(optarg);break;
    }
    }

    /*if(optind==argc) {
                      fprintf(stderr," Missing URL!\n");
		      usage();
		      return 2;
                    }*/
    if(clients <= 300) 
    {
        threadnum = clients;
        threadalive = threadnum;
        processnum = 1;
    }
    else if(clients <= 10000)
    {
        threadnum = 100;
        threadalive = threadnum;
        processnum = clients/threadnum;
    }
    else
    {
        threadnum = 100;
        threadalive = threadnum;
        processnum = clients/threadnum;
    }
    if(runtime==0) runtime=5;
    /* Copyright */
    fprintf(stderr,"ADDRServerTest - "PROGRAM_VERSION"\n"
        "Copyright (c) yus 2013-2014. \n"
        );
    /* print tset info */
    printf("%d clients",clients);
    printf(", running %d sec", runtime);
    if(host!=NULL)
    {
        if(TCP)
            printf(", connect to remote server %s:%d(TCP),",host,port);
        if(UDP)
            printf("connect to remote server %s:%d(UDP)",host,port+1);
    } 
    printf(".\n");
    signal(SIGINT,int_handler);
    return test();
}

/* vraci system rc error kod */
static int test(void)
{
  int i,j,l,n;
  uint64_t k;	
  SInt64 m;
  pid_t pid=0;
  FILE *f;

  /* create pipe */
  if(pipe(mypipe))
  {
	  perror("pipe failed.");
	  return 3;
  }
  /* fork childs */
  for(i=0;i<processnum;i++)
  {
	   pid=fork();
	   if(pid <= (pid_t) 0)
	   {
		   /* child process or error*/
	           sleep(1); /* make childs faster */
		   break;
	   }
  }

  if( pid< (pid_t) 0)
  {
      fprintf(stderr,"problems forking worker no. %d\n",i);
	  perror("fork failed.");
	  return 3;
  }

  if(pid== (pid_t) 0)
  {
    /* I am a child*/
    // setup alarm signal handler
    
    struct sigaction sa1; 
    sa1.sa_handler=alarm_handler;
    sa1.sa_flags=0;
    if(sigaction(SIGALRM,&sa1,NULL))
    {
        printf("set signal error\n");
        exit(3);
    }
     /* setup int signal handler */
     alarm(runtime);
    //testcore(host,port,m_type);
     pthread_t* threadid = new pthread_t[threadnum];
    /* struct result_set
     {
        uint32_t success;
        uint32_t failed;
        uint32_t bytes;
        uint32_t totaltry;
     }res_set[]*/
     for(int i=0;i < threadnum;i++)
     {
        if(pthread_create(&threadid[i],NULL,pthread_testcore,NULL) < 0)
             printf("create thread error,errno:%d\n",errno);
        else
        {
            totalthread++;
        }
     }
    Assert(totalthread == threadnum);
    mutex.Lock();
    fCond.Wait(&mutex);
    mutex.Unlock();
    Assert(threadalive == 0);
    alarm(0);
    for(int i=0;i < threadnum ; i++)
    {    
        if(pthread_join(threadid[i],NULL) <0)
        {
            printf("pthread_join erro\n");
            break;
        }
    }
    /* write results to pipe */
    close(mypipe[0]);
    f=fdopen(mypipe[1],"w");
    if(f==NULL)
    {
        perror("open pipe for writing failed.");
        return 3;
    }
    /* fprintf(stderr,"Child - %d %d\n",success,failed); */
    fprintf(f,"%d %d %llu %d %lld %d\n",success,failed,bytes,totaltry,process_time,ADDRServerTest::ReadError);
    if(fclose(f) < 0)
        printf("error:%s,close stream failed\n",strerror(errno));
    delete [] threadid;
    return 0;
  } else
  {
      begin_time = OS::Milliseconds();
	  f=fdopen(mypipe[0],"r");
	  if(f==NULL) 
	  {
		  perror("open pipe for reading failed.");
		  return 3;
	  }
	  setvbuf(f,NULL,_IONBF,0);
	  success=0;
      failed=0;
      bytes=0;
      totaltry = 0;
       // fd_set fds;
	  while(1)
	  {
          pid=fscanf(f,"%d %d %" PRIu64 "%d %lld %d",&i,&j,&k,&l,&m,&n);
		  if(pid<2)
                  {
                       fprintf(stderr,"Some of our childrens died.\n");
                       break;
                  }
		  success += i;
		  failed += j;
		  bytes += k;
         totaltry += l;
         process_time += m;
         ReadError += n;
		  /* fprintf(stderr,"*Knock* %d %d read=%d\n",success,failed,pid); */
		  --processnum;
          if(processnum==0) break;
	  }
      end_time = OS::Milliseconds();
      real_runtime = (double)(end_time - begin_time)/1000;
      printf("\ntotaltry=%d(%d times/s),receive=%" PRIu64 " KB,speed=%d KB/sec,average session time=%f msec.\n" 
              "Requests: %d success, %d failed(%d error),successrate=%f,elapse=%lf sec\n",
            (int)totaltry,
            (int)((totaltry)/(real_runtime)),
            bytes/1024,
		    (int)(((float)bytes/real_runtime/1024)),
            (float)process_time/success,
		    success,
		    failed,
            ReadError,
            (float)success/(success+failed), 
            real_runtime);
      close(mypipe[0]);
      close(mypipe[1]);
  }
  return i;
}

static void* pthread_testcore(void*)
{
    testcore(host,port,m_type);
    mutex.Lock();
    if(--threadalive == 0)
        fCond.Signal();
    mutex.Unlock();
    pthread_exit(NULL);
}
static void testcore(const char *host,const int port,int type)
{
    int enum_m_type;
    switch(type)//set message type
    {
        case 0:
            enum_m_type = CLIENT_PUT;
            break;
        case 1:
            enum_m_type = CLIENT_GET;
            break;
        case 2:
            enum_m_type = NS_COMMAND_CHUNK_DEL;
            break;
        case 3:
            enum_m_type = NS_COMMAND_CHUNK_BACKUP;
            break;
        case 4:
            enum_m_type = 0;
            break;
        case 8:
            enum_m_type = 0;
            break;
        case 9:
            enum_m_type = 0;
            break;
        default: 
            enum_m_type = UNKNOWN;
    }
    struct in_addr addr;
    inet_aton(host, &addr);
    UInt32 hostip = ntohl(addr.s_addr);
    sigset_t new_mask,old_mask;
    if(sigprocmask(0, NULL,&new_mask) < 0)
        printf("get pending mask error\n"); 
    sigaddset(&new_mask,SIGALRM);
    SInt64      thread_time=0;
    int32_t     thread_try=0;
    int32_t     thread_success = 0;
    int32_t     thread_failed = 0;
    uint32_t     thread_bytes = 0;
    
    if(TCP)
    {
        int re = 0;
        SInt64 begin = OS::Milliseconds();
        while(!timerexpired)
        {
            ADDRServerTest TestObj(enum_m_type,hostip,port,inputfile);
            re=TestObj.TCPSession();
            if(re==0)
            {
                if(TestObj.successflag)
                {
                    thread_success++;
                    thread_bytes += TestObj.get_receive_bytes();
                    SInt64 end = OS::Milliseconds();
                    thread_time += (end-begin);
                }
            } 
            if(!TestObj.successflag)
            {
                thread_failed++; 
            }
            if(!loop)
                break;
            else
            {
                srand((int)time(NULL));
                int x = rand()%1000 + 1;
                usleep(x);
            }
        }
    }
    else if(UDP)
    {  
        ADDRServerTest TestObj(enum_m_type,hostip,port+1,inputfile);
        int re = 0;
        SInt64 begin = OS::Milliseconds();
        while(!timerexpired)
        {
            if(sigprocmask(SIG_BLOCK, &new_mask,&old_mask)) 
            { 
                printf("block signal SIGQUIT error\n"); 
            }  
            re=TestObj.UDPSession();
            if(sigprocmask(SIG_SETMASK, &old_mask,NULL) < 0)
                printf("recover pending mask error\n"); 
            if(re==0&&TestObj.successflag)
            {
                SInt64 end = OS::Milliseconds();
                thread_time += (end-begin);
                thread_success++;
                thread_bytes += TestObj.get_receive_bytes();
            }
            else
            {
                thread_failed++;
            }
            if(!loop)
                break;
            else
            {
                srand((int)time(NULL));
                int x = rand()%1000 + 1;
                usleep(x);
            }
        }
    }
    mutex.Lock();
    process_time += thread_time;
    bytes += thread_bytes;
    success += thread_success;
    failed += thread_failed;
    totaltry += thread_try;
    mutex.Unlock();
    return;
}

int ADDRServerTest::TCPSession()
{
    TCPSocket fSocket(NULL,Socket::kNonBlockingSocketType);
    OS_Error theErr;
    if((theErr = fSocket.Open()) != OS_NoErr)
    {
        if(debuglevel>=WarningVerbosity)
            qtss_printf("create tcp socket failed.\n");
        return -1;
    }
    if((theErr = fSocket.Connect(remote_ip,remote_port)) != OS_NoErr)
    {
        if(errno == EADDRNOTAVAIL)
        {
            fSocket.Cleanup();
            return 1;
        }
        else if((theErr == EINPROGRESS) || (theErr == EAGAIN))
        {
            fd_set wset;
            struct timeval tval;
            FD_ZERO(&wset);     
            FD_SET(fSocket.GetSocketFD(), &wset);     
            tval.tv_sec = 10;     
            tval.tv_usec = 0;   
            if (select(fSocket.GetSocketFD()+1, NULL, &wset,NULL,&tval) == 0) 
            {     
                fSocket.Cleanup();
                if(debuglevel>=WarningVerbosity)
                    printf("connnect timeout\n");
                return -1;
            }
        }
        else
        {
            fSocket.Cleanup();
            if(debuglevel>=WarningVerbosity)
                qtss_printf("connect remote host failed,%s,errno:%d.\n",strerror(errno),errno);
            return -1;
        }
    }
    theErr = fSocket.SetSocketRcvBufSize(TCPRCVWINSIZE);
    //get tcp sockopt Rcv buffer
    
    socklen_t optlen,
    rcvbuf_size;
    optlen = sizeof(rcvbuf_size);
    getsockopt(fSocket.GetSocketFD(), SOL_SOCKET, SO_RCVBUF,&rcvbuf_size, &optlen);
    //printf("Sock_opt:TCP RCV window size:%d,recv buf size: %d\n",rcvbuf_size,recvbufferlen);
    
    if(theErr != OS_NoErr)
        printf("Set Recv buffer error\n");
    PacketMessage();
    //1.send request
    UInt32 theLengthSent; 
    do
    {
        theLengthSent = 0;
        // Loop, trying to send the entire message.
        theErr = fSocket.Send(sendbuf + sendbytes,sendbuflen - sendbytes, &theLengthSent);
        sendbytes += theLengthSent; 
       
    } while (theLengthSent > 0);
    if(theErr != OS_NoErr)
    {
        if(debuglevel>=WarningVerbosity)
            qtss_printf("sendmessage error,%s\n",strerror(theErr));
        return 0;
    }
   
    //2.send data to server,if upstream to server
    if(messagetype == CLIENT_PUT)
    {
        if(NULL == inputfile)
        {
            qtss_printf("Argument error,file name not specified when upstream to server\n");
            return 0;
        }
        int filefd = open(inputfile,O_RDONLY,0644);
        if(filefd < 0)
        {
            printf("open in file failed\n");
            return -1;
        }
        int rc= 0;
        off_t offset = c_put->start;
        size_t size = c_put->end-c_put->start+1;
        size_t totalsend = 0,copy_length = size;
        do{
            rc = ::sendfile(fSocket.GetSocketFD(),filefd,&offset,copy_length);
            if(rc < 0)
            {
                if(errno == 11)
                {
                    usleep(100);
                    continue;
                }
                break;
            }
            copy_length -= rc;
            totalsend += rc;
        }while(copy_length > 0);
        if(debuglevel >= DebugVerbosity)
            printf("total %u bytes is sent with sendfile,filename:%s\n",totalsend,inputfile);
        close(filefd);
    }
    
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    //setsockopt(fSocket.GetSocketFD(),SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));    
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(fSocket.GetSocketFD(), &rfds);
    int retval;   
    if((retval = select(fSocket.GetSocketFD()+1, &rfds, NULL, NULL, &tv))==0)
    {
        if(debuglevel>=WarningVerbosity)
            printf("read timeout\n");
        return 0;
    }
    else if(retval < 0)
    {
        if(debuglevel>=WarningVerbosity)
            printf("select read error\n");
        return 0;
    }
    TCPRead(fSocket);
    if(ParseMessage(&fSocket) < 0)
        return -1;
    return 0;
}

int ADDRServerTest::TCPRead(TCPSocket& fSocket)
{
    OS_Error theErr;
    UInt32 s_len;
    do
    {
        s_len = 0;
        Assert(receivebytes < recvbufferlen);
        if((theErr=fSocket.Read(recvbuf + receivebytes,recvbufferlen,&s_len)) != OS_NoErr)
        {
            if(theErr == EAGAIN)
            {
                usleep(100);
                continue;
            }
            if(debuglevel>=WarningVerbosity)
                qtss_printf("TCP receive message error,errno:%d.\n",theErr);
            return -1;
        }
        else
        {
            receivebytes += s_len;
        }
        if( !recvheadlen && receivebytes >= 4)
        {   
            recvheadlen = (UInt32)GetRequestLen(recvbuf);
        }
        if( receivebytes >=  recvheadlen)
        {    
            successflag=true;
            break;
        }
    }while(1);
    return 0;
}

int ADDRServerTest::UDPSession()
{
    UDPSocket fSocket(NULL,0);//Socket::kNonBlockingSocketType
    if(fSocket.Open() != OS_NoErr)
    {
        if(debuglevel>=WarningVerbosity)
            qtss_printf("create udp socket failed.\n");
        return -1;
    }
    PacketMessage();
    if(fSocket.SendTo(remote_ip,remote_port,sendbuf,sendbuflen) != OS_NoErr)
    {
        if(debuglevel>=WarningVerbosity)
            qtss_printf("sendmessage error.\n");
        return -1;
    }
    else
    {
       sendbytes = sendbuflen;     
    }
    int retval;
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(fSocket.GetSocketFD(), &rfds);
    retval = select(fSocket.GetSocketFD()+1, &rfds, NULL, NULL, &tv);
    UInt32 s_len = 0;
    if (retval == -1)
    {
        if(debuglevel>=WarningVerbosity)
            perror("select()");
        return -1;
    }
    else if (retval && FD_ISSET(fSocket.GetSocketFD(), &rfds))
    {
        UInt32 m_ip;
        UInt16 m_port;
        if(fSocket.RecvFrom(&m_ip,&m_port,recvbuf,recvbufferlen,&s_len) != OS_NoErr)
        {
            if(debuglevel>=WarningVerbosity)
                qtss_printf("UDP receive message error.\n");
        }
        else
        {
            //Assert(m_ip == remote_ip);
            receivebytes = s_len;
            successflag=true;
            if(ParseMessage(&fSocket) < 0)
                return -1;
        }
    }
    else
    {
        if(debuglevel>=WarningVerbosity)
        {
            printf("UDP timeout\n");
        }
    }
    return 0;
}

void  ADDRServerTest::PacketMessage()
{
    if(RequestID > 65532) RequestID = 0;
    if(debuglevel >= DebugVerbosity)
        printf("send messagetype:%x\n",messagetype);
    switch(messagetype)
    {
        case CLIENT_PUT: 
        {
            c_put = (ClientPut*)malloc(44+20*3);//upload a whole block
            c_put->type = CLIENT_PUT;
            c_put->request_id = 1;
            c_put->backup_dataservernum = 3;
            c_put->length = c_put->backup_dataservernum*20 + 44;
            c_put->fileid = fileid;
            c_put->chunkid = chunkid;
            c_put->encrypt = 0;
            c_put->start = 0;
            c_put->end = 1024*1024*20-1;
            inet_pton(AF_INET, backhost1, c_put->address[0].ip);
            int32_t ip4= ntohl(*(int32_t*)c_put->address[0].ip);
            memcpy(c_put->address[0].ip,&ip4,sizeof(int32_t));
            c_put->address[0].port = 5800;
            
            inet_pton(AF_INET, backhost2, c_put->address[1].ip);
            ip4= ntohl(*(int32_t*)c_put->address[1].ip);
            memcpy(c_put->address[1].ip,&ip4,sizeof(int32_t));
            c_put->address[1].port = 5800;
            
            inet_pton(AF_INET, backhost3, c_put->address[2].ip);
            ip4= ntohl(*(int32_t*)c_put->address[2].ip);
            memcpy(c_put->address[2].ip,&ip4,sizeof(int32_t));
            c_put->address[2].port = 5800;
            
            int64_t pos = 0;
            c_put->serialize(sendbuf,MAXsendbuf,pos);
            sendbuflen = pos;
            break;
        }
        case CLIENT_GET:
        {
            c_get = (ClientGet*)malloc(sizeof(ClientGet));
            c_get->type = CLIENT_GET;
            c_get->request_id = 1;
            c_get->length = sizeof(ClientGet);
            c_get->fileid = fileid;
            c_get->chunkid = chunkid;
            c_get->encrypt = 0;
            c_get->start = 0;
            c_get->end = 1024*1024*20-1;
            int64_t pos = 0;
            c_get->serialize(sendbuf,MAXsendbuf,pos);
            sendbuflen = pos;
            break;
        }
        case NS_COMMAND_CHUNK_DEL:
        {
            ns_del.length = sizeof(DeleteCommand);
            ns_del.request_id = 0;
            ns_del.type = NS_COMMAND_CHUNK_DEL;
            ns_del.encrypt = 0;
            ns_del.fileid = fileid;
            ns_del.chunkid = chunkid;
            sendbuflen = ns_del.length;
            int64_t pos = 0;
            ns_del.serialize(sendbuf,MAXsendbuf,pos);
            break;
        }
        case NS_COMMAND_CHUNK_BACKUP:
        {
            ns_backup.length = sizeof(BackupCommand);
            ns_backup.request_id = 0;
            ns_backup.type = NS_COMMAND_CHUNK_BACKUP;
            ns_backup.encrypt = 0;
            ns_backup.fileid = fileid;
            ns_backup.chunkid = chunkid;
    
            struct in_addr addr;
            inet_aton(backhost1, &addr);
            int32_t ip4 = ntohl(addr.s_addr);
            memcpy(ns_backup.dst_address.ip,&ip4,4);
            ns_backup.dst_address.port = backport1;
            sendbuflen = ns_backup.length;
            int64_t pos = 0;
            ns_backup.serialize(sendbuf,MAXsendbuf,pos);
            break;
        }
        case UNKNOWN:
        default:
            break;
    }
}

int ADDRServerTest::ParseMessage(Socket* fSocket)
{
   
    if(CheckRecvMessgae(recvbuf,receivebytes) < 0)
    {     
        printf("bad message\n");
        BadPacket++;
        return -1;
    }
    int type = GetRequestType(recvbuf);
    switch(type)
    {
        case CLIENT_PUT:
        {
            
            ClientPutRes c_put;
            int64_t pos = 0;
            c_put.deserialize(recvbuf,receivebytes,pos);
            if(debuglevel >= DebugVerbosity)
            {
                printf("\nPUT resp from server:\n");
                c_put.dump();
            }
            break;
        }
        case CLIENT_GET:
        {
            ClientGetRes res;
            int64_t pos = 0;
            res.deserialize(recvbuf,receivebytes,pos);
            if(debuglevel >= DebugVerbosity)
            {
                printf("\nGET resp from server:\n");
                res.dump();
            }
            int filefd = -1;
            if(inputfile)
            {
                printf("write data to file %s\n",inputfile);
                filefd = open(inputfile,O_WRONLY|O_CREAT|O_APPEND|O_TRUNC,0644);
                if(filefd < 0)
                {
                    printf("open out file failed,%s\n",strerror(errno));
                    return -1;
                }
            }
            int re=TCPReadDataToFile(fSocket,filefd,c_get->end-c_get->start+1);
            if(filefd) close(filefd);
            return re;
        }
        case UNKNOWN:
        default:
            printf("\n UNKNOWN resp from server:\n");
            UnKnownPacket++;
            return -1;
    }
    return 0;
}

int ADDRServerTest::TCPReadDataToFile(Socket* fSocket,int filefd,int datalen)
{
    successflag=false;
    int preread=receivebytes-recvheadlen;
    if(preread)
    {
        if(filefd > 0&&write(filefd,recvbuf+recvheadlen,receivebytes-recvheadlen) < 0)
            printf("write data to file:%s error,%s,when preread\n",outfile,strerror(errno));
    }
    datalen -= preread;
    if(datalen <=0)
    {
        successflag=true;
        return 0;
    }
    OS_Error theErr;
    UInt32 s_len;
    SInt64 timeout = 5000;//5s
    LastReadTime = OS::Milliseconds();
    do
    {
        s_len = 0;
        if((theErr=fSocket->Read(recvbuf,recvbufferlen,&s_len)) != OS_NoErr)
        {
            if(theErr == EAGAIN)
            {
                if(OS::Milliseconds() - LastReadTime > timeout)
                {
                    ReadTimeOutError++;
                    if(debuglevel>=WarningVerbosity)
                        qtss_printf("%d client read data timeout.\n",ReadTimeOutError);
                    break;
                }
                usleep(100);
                continue;
            }
            if(debuglevel>=WarningVerbosity)
                qtss_printf("TCP receive message error(%d),%d bytes data received.\n",theErr,receivebytes-recvheadlen);
            ReadError++;
            return -1;
        }
        else
        {
            receivebytes += s_len;
            datalen -= s_len;
            LastReadTime = OS::Milliseconds();
            if(filefd > 0 && write(filefd,recvbuf,s_len) < 0)
            {
                printf("write data to file:%s error,when receive data,%s\n",outfile,strerror(errno));
                break;
            }
        }
        if( datalen <= 0)
        {    
            successflag=true;
            break;
        }
    }while(!timerexpired);
    if(debuglevel >= DebugVerbosity)
        printf("%d bytes data received,recvheadlen:%d\n",receivebytes-recvheadlen,recvheadlen);
    return 0;
}

