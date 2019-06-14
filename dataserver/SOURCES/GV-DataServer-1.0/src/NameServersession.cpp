#include <sys/types.h>
#include <netdb.h>
#include "OS.h" 
#include "defaultpref.h"
#include "DSServerPref.h"
#include "NameServersession.h"
#include "DSErrorLog.h"
#include "DSMessageProcess.h"
#include "data_management.h"
#include "DSServer.h"
#include <sys/syscall.h> 
#define gettid() syscall(__NR_gettid) 
#define MAXBANDWIDTH (1024*1024*1024)
extern tfs::dataserver::DataManagement* datamanager;
extern ServerPref* svrPref;
#define NS_DEBUG 1
//if error retry,return IdleTime,if heatbeat setidletask,else if return 0 for read command
SInt64 NameServerSession::Run()
{
    EventFlags events = this->GetEvents();
    if (events & Task::kKillEvent)  return -1;
    OS_Error theErr;
    if(fState == kInitial)
    { 
        if(!fSocket)
            fSocket = new TCPServerSocket(Socket::kNonBlockingSocketType);
        if((theErr = fSocket->Open()) != OS_NoErr)  
        {
            LogError(WarningVerbosity,"Open socket error,while try to connect nameserver");
            return theErr;
        }
        packet_dataserver_resgister();
        fSocket->SetTask(this);
        theErr = fSocket->Connect(ntohl(NameServerIP),NameServerPort);
        if(theErr != OS_NoErr)
        {
            if((theErr == EINPROGRESS) || (theErr == EAGAIN))
            { 
                fSocket->RequestEvent(EPOLLOUT);
                fState = kSendRegister;
                return 0;
            } 
            else
            {
                again();
                LogError(MessageVerbosity,"connect to NameServer(ip:%s,port:%d) failed\n",NameServerIPStr,NameServerPort);
                return IdleTime;
            }
        } 
        else
        {
            fState = kSendRegister;
            LogError(MessageVerbosity,"connect to NameServer(ip:%s,port:%d) success\n",NameServerIPStr,NameServerPort);
        }
    }
    if(fState == kSendRegister)
    {
        theErr = fSocket->Sendall(fPacketSend,fPacketSendLen);
        if(theErr == EAGAIN)
        {
            fPacketSendLen = 0;
            fSocket->RequestEvent(EPOLLOUT|EPOLLET);
            return 0;
        }
        if(theErr != OS_NoErr)
        {
            LogError(WarningVerbosity,"Send register request to NameServer:%s error:%s\n",NameServerIPStr,strerror(theErr));
            again();
            return IdleTime; 
        } 
        fState = kRecvRegisterResponse;
    }
    if(fState == kRecvRegisterResponse)
    {
        do
        {
            UInt32 theRecvLen = 0;
            theErr = fSocket->Reads(&fRecvBuffer[fRecvLen], kReqBufSize - fRecvLen, &theRecvLen); 
            fRecvLen += theRecvLen;
            if(fRecvLen >= (UInt32)GetRequestLen(fRecvBuffer)&&fRecvLen > 0)
            {
                fRequestLen = GetRequestLen(fRecvBuffer);
                if(MessageUtility::MessageTransaction(fSocket,fRecvBuffer,fRequestLen) < 0)
                {
                    again();
                }
                memcpy(fRecvBuffer,fRecvBuffer+fRequestLen,fRecvLen-fRequestLen); 
                LogError(DebugVerbosity,"Receive Nameserver <%s> response,dataserver online success",NameServerIPStr);
                fRecvLen -= fRequestLen;
                break;
            }  
        }while(theErr == OS_NoErr);
        if(theErr == EAGAIN)
        {
            fSocket->RequestEvent(EPOLLIN);
            return 0;
        }
        if (theErr != OS_NoErr)
        {
            LogError(WarningVerbosity," Receive Nameserver:%s response failed,error:%s\n",NameServerIPStr,strerror(theErr));
            again();
            return IdleTime; 
        }
        this->SetIdleTimer(IdleTime);
        fState = kConnected;
        fSocket->RequestEvent(EPOLLIN);
    } 
    if(fState == kConnected) 
    {
        if (events & Task::kReadEvent)//read command
        {
            do
            {
                UInt32 theRecvLen = 0;
                theErr = fSocket->Reads(&fRecvBuffer[fRecvLen], kReqBufSize - fRecvLen, &theRecvLen); 
                fRecvLen += theRecvLen;
                while(fRecvLen > 0&&fRecvLen >= (UInt32)GetRequestLen(fRecvBuffer))
                {
                    fRequestLen = GetRequestLen(fRecvBuffer);
                    if(MessageUtility::MessageTransaction(fSocket,fRecvBuffer,fRequestLen) < 0)
                    {
                        again();
                    }
                    memcpy(fRecvBuffer,fRecvBuffer+fRequestLen,fRecvLen-fRequestLen);
                    fRecvLen -= fRequestLen;
                }
                
            }while(theErr == OS_NoErr);
            if (theErr != OS_NoErr&&theErr != EAGAIN)
            {
                LogError(WarningVerbosity," Receive Nameserver:%s command failed,error:%s\n",NameServerIPStr,strerror(theErr));
                again();
                return IdleTime; 
            }
        }
        if (events & Task::kIdleEvent)//send heatbeat
        {
            if(MulIdleTime >= HeartBeatTime)
            {
                MulIdleTime = 0;
                LogError(DebugVerbosity,"Send Heartbeat to NameServer:%s\n",NameServerIPStr);
                packet_dataserver_heatbeat();
                theErr = fSocket->Sendall(fPacketSend,fPacketSendLen);
                if(theErr == EAGAIN)
                {
                    return 0;
                }
                if(theErr != OS_NoErr)
                {
                    LogError(WarningVerbosity,"Send Heartbeat to NameServer:%s failed,%s\n",NameServerIPStr,strerror(theErr));
                    again();
                    return IdleTime; 
                }
            }
            MulIdleTime += IdleTime;
            ADDRServer::GetServer()->CalcBindWidth();
            this->SetIdleTimer(IdleTime);
        }
        if (events & Task::kWriteEvent||sendbusy) 
        {
            do
            { 
                if(!sendbusy)
                {
                    GetMessageFromQueue(ResMessage,ResMessageLen);
                    if(NULL == ResMessage)
                    {
                        fSocket->RequestEvent(EPOLLIN);
                        break;    
                    }
                }
                if(ResMessage)
                {
                    theErr = fSocket->Sendall(ResMessage,ResMessageLen);
                    if(theErr == EAGAIN)
                    {
                        sendbusy = true;
                        fSocket->RequestEvent(EPOLLIN|EPOLLOUT|EPOLLET);
                        return 0;
                    }
                    //success or error
                    sendbusy = false;  
                    delete ResMessage;
                    ResMessage = NULL;
                    if(theErr != OS_NoErr)
                    {
                        LogError(WarningVerbosity,"Send Command Result to NameServer:%s failed,%s\n",NameServerIPStr,strerror(theErr));
                        again();
                        return IdleTime;
                    }else
                    {
                        LogError(MessageVerbosity,"Send Command Result to NameServer:%s\n",NameServerIPStr);
                    }
                }
            }while(1);
        }
    }
    return 0;
}

void NameServerSession::again()
{
    fState = kInitial;
    sendbusy = false;
    if(fSocket)
    {
        delete fSocket;
        fSocket = NULL;
    }
}

void NameServerSession::GetNameServerIp(UInt32 iplist[],UInt16 * numofip)//domain to network order iplist
{
    if(iplist == NULL||numofip == NULL) return; 
    struct hostent *hp;
    char** p;
    hp=gethostbyname(svrPref->GetNameServerDomain());
    if(hp==NULL)
    {
#if __debug__
         (void)printf("host information for %s not found\n",svrPref->GetNameServerDomain());
#endif       
         return;
    }
    int i=0;
    for(p=hp->h_addr_list;*p!=0;p++,i++)
    {
        (void)memcpy(&iplist[i],*p,hp->h_length);  
    }
    *numofip = i;
}

int NameServerSession::packet_dataserver_resgister()
{
    int64_t pos = 0;
    DsRegister reg;
    reg.type = DS_REGISTER;
    reg.request_id = GenerateRequestId();
    reg.length = sizeof(DsRegister);
    reg.encrypt = 0;
    reg.servicetype = DATASERVICE;
    reg.dataserverid = svrPref->GetDataserverId();
    int32_t blockcount = 0;
    int64_t diskused = 0; 
    datamanager->get_ds_filesystem_info(blockcount,diskused,reg.diskcapacity);
    reg.diskfree = (reg.diskcapacity - diskused)>>10;
    reg.diskcapacity >>= 10;
    reg.port = svrPref->GetLocalPort();
    fPacketSendLen = reg.length;
    if(reg.serialize(fPacketSend, kReqBufSize, pos) < 0)
        return -1;
    return 0;
}

int NameServerSession::packet_dataserver_heatbeat()
{
    DsHeartBeat request;
    request.length = sizeof(DsHeartBeat);
    request.type = DS_HEARTBEAT;
    request.request_id = 0;
    request.encrypt = 0;
    request.servicetype = DATASERVICE;
    request.loadweight = calc_load_weight();
    if(request.loadweight<0||request.loadweight>100)
        LogError(FatalVerbosity,"loadwight:%d is illegal",request.loadweight);
#if NS_DEBUG
    request.dump();
#endif
    fPacketSendLen = request.length;
    int64_t pos = 0;
    if(request.serialize(fPacketSend, kReqBufSize, pos) < 0)
        return -1;
    return 0;
}
int32_t NameServerSession::calc_load_weight()
{
    UInt32 RcvBandwidthInBits = ADDRServer::GetServer()->GetCurRcvBandwidthInBits();
    UInt32 SndBandwidthInBits = ADDRServer::GetServer()->GetCurSndBandwidthInBits();
#if NS_DEBUG
    printf("RcvBandwidthInBits:%u MB/s\n",RcvBandwidthInBits/(8*1024*1024));
    printf("SndBandwidthInBits:%u MB/s\n",SndBandwidthInBits/(8*1024*1024));
#endif
    float usage_bandwidth1 = (float)RcvBandwidthInBits/MAXBANDWIDTH;
    float usage_bandwidth2 = (float)SndBandwidthInBits/MAXBANDWIDTH;
    int32_t blockcount = 0;
    int64_t diskused = 0; 
    int64_t diskcapacity =0;
    datamanager->get_ds_filesystem_info(blockcount,diskused,diskcapacity);
    float usage_disk = (float)diskused/diskcapacity;
    float usage = (usage_bandwidth1+usage_bandwidth2+usage_disk)/3;
    if(usage_bandwidth1 > 0.8||usage_bandwidth2 > 0.8||usage_disk>0.8)
        return 100*(0.8+usage*0.2);
    else
        return 100*usage;
}
void NameServerSession::AddMessageToQueue(char* fPacket,UInt32 fPacketLen)
{
    OSQueueElem* fMessageQueueElem = new OSQueueElem;
    StrPtrLen* obj = new StrPtrLen(fPacket,fPacketLen);
    fMessageQueueElem->SetEnclosingObject(obj);
    fMessageQueue.EnQueue(fMessageQueueElem);
}

void  NameServerSession::GetMessageFromQueue(char*& fPacket,UInt32& fPacketLen)
{
    OSQueueElem* fMessageQueueElem = fMessageQueue.DeQueue();
    if(NULL == fMessageQueueElem)
    {
        fPacket = NULL;
        return;
    }
    StrPtrLen* obj = (StrPtrLen*)fMessageQueueElem->GetEnclosingObject();
    fPacket = obj->Ptr;
    fPacketLen = obj->Len;
    delete fMessageQueueElem;
    delete obj;
}
void  NameServerSession::ClearMessageFromQueue()
{
     char* fPacket;
     UInt32 fPacketLen = 0;
     do{
        GetMessageFromQueue(fPacket,fPacketLen);
        if(NULL == fPacket)
            break;
        delete fPacket;
     }while(1);
}