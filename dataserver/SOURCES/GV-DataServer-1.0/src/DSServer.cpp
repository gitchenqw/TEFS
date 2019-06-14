
#include <signal.h>

#include "OS.h"
#include "TimeoutTask.h"
#include "DSClientPutTask.h"
#include "NameServersession.h"
#include "data_management.h"
#include "DSServer.h"

extern ServerPref* svrPref;
extern tfs::dataserver::DataManagement* datamanager;
extern NameServerSession*   NSsession;

Bool16 sHasPID = false;
ADDRServer* ADDRServer::Server = NULL;
static int set_fs_param(tfs::common::FileSystemParameter &fs_param)
{
  char* MountRootPath = svrPref->GetMountRootPath();
  if(NULL == MountRootPath)
      return -1;
  fs_param.mount_name_.assign(MountRootPath);
  fs_param.max_mount_size_ = 20971520;
  fs_param.base_fs_type_ = 3;
  fs_param.super_block_reserve_offset_ = 0;
  fs_param.avg_segment_size_ = 15 * 1024;  //15K
  fs_param.main_block_size_ = 1024*1024*64;    //16M
  fs_param.extend_block_size_ = 1048576;   //1M
  fs_param.block_type_ratio_ = 0.5;
  fs_param.file_system_version_ = 1;
  fs_param.hash_slot_ratio_ = 0.5;
  return 0;
}

void ADDRServer::Initialize( ServerType type,UInt16 thePort,bool DontFork,UInt32 verbose)
{
    SessionType = type;
    inDontFork = DontFork;
    verboseLevel = verbose;
    char* ip = svrPref->GetLocalIp();
    if(ip)
    {
        setaddr(ntohl(inet_addr(ip)),thePort);
    }
    else
    {
        setaddr(INADDR_ANY,thePort);
    }
    tfs::common::FileSystemParameter fs_param;
    set_fs_param(fs_param);
    datamanager = new tfs::dataserver::DataManagement();
    int ret = datamanager->init_block_files(fs_param);
    if(ret != GV_SUCCESS)
    {
        LogError(FatalVerbosity,"File system init failed");
        exit(0);
    }
}

void ADDRServer::setaddr( UInt32 address,UInt16 listen_port)
{
    TCPaddr = address;
    TCPport = listen_port;
    UDPaddr = address;
    UDPport = listen_port+1;
}

void ADDRServer::StartListeners()
{
    // Start listening
    if(SessionType == kTCPServerType)
    {
        fTCPListeners = new ADDR_TCPListenerSocket* [fTCPNumListeners];
        for (UInt32 x = 0; x < fTCPNumListeners; x++)
        {   
            fTCPListeners[x] = new ADDR_TCPListenerSocket();
            if(fTCPListeners[x]->Initialize(TCPaddr,TCPport) != OS_NoErr)
            {
                qtss_printf("%s, %d (tcp listener initial error)\n",__FILE__,__LINE__);
                exit(0);
            }
#ifdef _EPOLL
            fTCPListeners[x]->RequestEvent(EPOLLIN|EPOLLET);
#else
            fTCPListeners[x]->RequestEvent(EV_RE);
#endif
        }
    }
    else if(SessionType == kUDPServerType)
    {
    }
}

void ADDRServer::StartServer()
{
    OS::Initialize();
    OSThread::Initialize();
    //signal(SIGINT,int_handler);
    UInt32 numShortTaskThreads = 0;
    UInt32 numBlockingThreads = 2;
    UInt32 numThreads = 0;
    if (numBlockingThreads == 0)
            numBlockingThreads = 1;
    if (numShortTaskThreads == 0)
        numShortTaskThreads = 2;

    numThreads = numShortTaskThreads + numBlockingThreads;
    //qtss_printf("Add threads shortask=%lu blocking=%lu\n",numShortTaskThreads, numBlockingThreads);
    TaskThreadPool::SetNumShortTaskThreads(numShortTaskThreads);
    TaskThreadPool::SetNumBlockingTaskThreads(numBlockingThreads);
    TaskThreadPool::AddThreads(numThreads); //start task thread;
    Socket::Initialize();
    SocketUtils::Initialize(!inDontFork);
    TimeoutTask::Initialize();
#ifdef _EPOLL
    ::epoll_startevents();
#else
    ::select_startevents();//initialize the select() implementation of the event queue
#endif
    IdleTask::Initialize();
    Socket::StartThread();
    NameServerSession::GetNameServerIp(NameServerIP,&NameServerNum);
    NSsession = new NameServerSession(BeatTime*1000,NameServerIP[0],svrPref->GetNameServerPort());
    StartListeners();
    CleanPid(true);
    WritePid(!inDontFork);
    LogError("#Remark: data server beginning STARTUP");
    in_addr in;
    
    if(SessionType == kTCPServerType)
    {
        in.s_addr = htonl(TCPaddr);
        LogError(MessageVerbosity,"Listen on %s:%d(TCP)",inet_ntoa(in),TCPport);
    }
    else
    {
        in.s_addr = htonl(UDPaddr);
        LogError(MessageVerbosity,"Listen on %s:%d(UDP)",inet_ntoa(in),UDPport);
    }
    /*LoadServerReport::getserverip(AddrServerIP,&AddrServerNum);
    for(int i=0;i < AddrServerNum;i++)
    {
        LoadServerReport* StatusReportOntime = new LoadServerReport(BeatTime*1000,AddrServerIP[i]);
    }*/
    do
    {
        OSThread::Sleep(1000*10);  
    }while(1);
}

void WritePid(Bool16 forked)
{
    // WRITE PID TO FILE
    char* thePidFileName = svrPref->GetPidFilePath();
    FILE *thePidFile = fopen(thePidFileName, "w");
    if(thePidFile)
    {
        if (!forked)
            fprintf(thePidFile,"%d\n",getpid());    // write own pid
        else
        {
            fprintf(thePidFile,"%d\n",getppid());    // write parent pid
            fprintf(thePidFile,"%d\n",getpid());    // and our own pid in the next line
        }                
        fclose(thePidFile);
        sHasPID = true;
    }
}

void CleanPid(Bool16 force)
{
#ifndef __Win32__
    if (sHasPID || force)
    {
        char* thePidFileName = svrPref->GetPidFilePath();
        unlink(thePidFileName);
    }
#endif
}

int ADDRServer::CalcBindWidth()
{
    unsigned int periodicRcvBytes = fPeriodicRcvBytes;
    (void)atomic_sub(&fPeriodicRcvBytes, periodicRcvBytes);
    // Same deal for bytes send
    unsigned int periodicSndBytes = fPeriodicSndBytes;
    (void)atomic_sub(&fPeriodicSndBytes, periodicSndBytes);
    
    SInt64 curTime = OS::Milliseconds();
    
    //also update current bandwidth statistic
    if (fLastBandwidthTime != 0)
    {
        Assert(curTime > fLastBandwidthTime);
        UInt32 delta = (UInt32)(curTime - fLastBandwidthTime);
		// Prevent divide by zero errror
		if (delta < 1000) {
			return 0;
		}
        
        UInt32 theTime = delta / 1000;
        Float32 bits = periodicRcvBytes * 8;
        bits /= theTime;
        fCurrentRcvBandwidthInBits = (UInt32) (bits);
        
        bits = periodicSndBytes * 8;
        bits /= theTime;
        fCurrentSndBandwidthInBits = (UInt32) (bits);
    }
    fLastBandwidthTime = curTime;
    return 0;
}




