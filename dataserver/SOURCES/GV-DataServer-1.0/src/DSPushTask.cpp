//push to other dataserver
#include <sys/types.h>
#include "OS.h" 
#include "Message.h"
#include "bserveripPref.h"
#include "DSPushTask.h"
#include "data_management.h"
#include "NameServersession.h"
#include "DSServer.h"
#include <sys/syscall.h> 

#define __debug__ 0
#define gettid() syscall(__NR_gettid) 

UInt32       DSPushTask::TotalTaskID = 0;
extern tfs::dataserver::DataManagement* datamanager;

DSPushTask::DSPushTask(char ip[16],int port,SInt64 fileid,SInt32 chunkid,SInt32 start,SInt32 end):Task(),
    fSocket(Socket::kNonBlockingSocketType),
    fState(kInitial),
    HasGenResp(false),
    fBufferLen(0),
    fPacketSendLen(0),
    read_offset(start),
    real_read_len(0),
    read_start(start),
    read_end(end),
    busy(false),
    fMask(EPOLLIN|EPOLLET)
    {
#if IPV6
        memcpy(RemoteIp,ip,INET_ADDRSTRLEN);
#else
        RemoteIp = *(UInt32*)ip;
#endif
        RemotePort = port;
        BlockInfo.fileid = fileid;
        BlockInfo.chunkid = chunkid;
        BlockInfo.blocksize = end-start+1;
        NeedSend = end-start+1;
        if(fSocket.Open() != OS_NoErr)  
            LogError(WarningVerbosity,"Open new socket error while push stream to server");
        fSocket.SetTask(this);
        this->Signal(kWriteEvent);
        fSocket.RequestEvent(fMask);
        this->SetThreadPicker(Task::GetBlockingTaskThreadPicker());
        this->SetTaskName("PushTask");
    }

SInt64 DSPushTask::Run()
{
#if __debug__
    printf("PushTask: %d come in\n",TaskID);
#endif
    EventFlags events = this->GetEvents(); 
    if (events & Task::kKillEvent)
        return -1;
     if (events & Task::kTimeoutEvent)
        return -1;
    OS_Error theErr = this->ProcessEvent(Task::kReadEvent);
    if((theErr == EINPROGRESS) || (theErr == EAGAIN))
    { 
        fSocket.RequestEvent(fMask);
        return 0;
    }
#if __debug__
    printf("PushTask: %d exit,%s\n",TaskID,strerror(theErr));
#endif
    return -1;
}

OS_Error DSPushTask::ProcessEvent(int /*eventBits*/)
{
    OS_Error theErr = OS_NoErr; 
    void* ip;
#if IPV6
    int domain = AF_INET6;
    //ip = RemoteIp;
#else 
    int domain = AF_INET;
    int32_t ip4 = htonl(RemoteIp);
    ip = &ip4;
#endif
    if(OS::NetWorkToPoint(domain,ip,RemoteIpStr) == NULL) 
        return theErr;
    do
    {
        if(fState == kInitial)
        {
#if IPV6
#else
            theErr = fSocket.Connect(RemoteIp,RemotePort);
#endif
            fState = kRequestSending;
            if(theErr == EAGAIN||theErr == EINPROGRESS)
            {
                fMask = EPOLLOUT;
                LogError(DebugVerbosity,"Wait for connecting,when push data to sever<%s:%d>",RemoteIpStr,RemotePort);
                return theErr;  
            }              
            if(theErr != OS_NoErr)
            {
                LogError(WarningVerbosity,"connect error,when push data to sever<%s:%d>,%s",RemoteIpStr,RemotePort,strerror(theErr));
                return theErr;
            }
        }
        if(fState == kRequestSending)
        {
            if(!HasGenResp)
            {
                if( packet_ds_backup_request() < 0)
                    LogError(DebugVerbosity,"packet ds push request error\n");
                HasGenResp = true;
            }
            LogError(DebugVerbosity,"Send push request to server:<%s:%d>,length:%d,id:%d,type:0x%x,encrypt:%s,blockid<%lld,%d>",
                    RemoteIpStr,RemotePort,M_Push.length,M_Push.request_id,M_Push.type,(M_Push.encrypt ? "yes":"no"),M_Push.fileid,M_Push.chunkid);
            theErr = fSocket.Sendall(fPacketSend,fPacketSendLen);
            if(theErr != OS_NoErr)
            {
                fMask = EPOLLOUT;
                return theErr;
            }
            fState = kDataSending;
            fMask = EPOLLOUT;
        } 
        if(fState == kDataSending)
        {
            do{
                if(!busy)
                {
                    real_read_len = kMaxPacketSize;
                    if(read_offset + real_read_len > read_end)
                        real_read_len = read_end - read_offset+1; 
                    if(datamanager->read_raw_data(BlockInfo.fileid,BlockInfo.chunkid,read_offset,
                                                real_read_len,fPacketSend) < 0)
                    {
                        LogError(DebugVerbosity,"read disk data error,length:%d,id:%d,type:0x%x,encrypt:%s,blockid<%ld,%d>\
                                  when push stream to server",M_Push.length,M_Push.request_id,M_Push.type,
                                  (M_Push.encrypt ? "yes":"no"),BlockInfo.fileid,BlockInfo.chunkid);
                        return theErr;
                    }
                    fPacketSendLen = real_read_len;
                }
                theErr = fSocket.Sendall(fPacketSend,fPacketSendLen); 
                if (theErr == EAGAIN)
                {
                    busy = true;
#if __debug__
                    LogError(DebugVerbosity,"Upstream data busy,%d bytes send\n",read_offset-real_read_len);
#endif
                    return theErr;
                }
                if(theErr != OS_NoErr)
                {
                    LogError(DebugVerbosity,"Send data error,when push stream to server<%s:%d>,blockid<%lld,%d>,%s,%d bytes sended",
                                RemoteIpStr,RemotePort,BlockInfo.fileid,BlockInfo.chunkid,strerror(theErr),read_offset);
                    return theErr;
                }
                ADDRServer::GetServer()->IncrementSndBytes(real_read_len);
                NeedSend -= real_read_len;
                read_offset += real_read_len;
                busy = false;
            }while(NeedSend>0);
            LogError(DebugVerbosity,"Push stream to server<%s:%d> complete,range:%d-%d,length:%d,blockid<%lld,%d>",
                     RemoteIpStr,RemotePort,read_start,read_offset,read_end-read_start+1,BlockInfo.fileid,BlockInfo.chunkid);
            fState = kResponseReceiving;
        }
        if(fState == kResponseReceiving)
        {
            UInt32 theRecvLen = 0;
            theErr = fSocket.Reads(&fRecvBuffer[fBufferLen], kReqBufSize - fBufferLen, &theRecvLen); 
            fBufferLen += theRecvLen;
            if (theErr == EAGAIN)
            {
                fMask = EPOLLIN;
                return theErr;
            }
            if(theErr != OS_NoErr)
            {
                LogError(DebugVerbosity,"Get PUSH response error,when upstream to server,blockid<%ld,%d>",
                                BlockInfo.fileid,BlockInfo.chunkid);
                return theErr;
            }
            if(fBufferLen >= 4)
            {
                if((int)fBufferLen >= GetRequestLen(fRecvBuffer))
                {
                    SInt64 pos = 0;
                    M_PushRes.deserialize(fRecvBuffer,kReqBufSize,pos);
#if __debug__
                    LogError(DebugVerbosity,"Get PUSH response from server<%s:%d>,blockid<%lld,%d>",
                             RemoteIpStr,RemotePort,BlockInfo.fileid,BlockInfo.chunkid);
                    M_PushRes.dump();
#endif
                    return OS_NoErr; 
                }
                else
                {
                    return EAGAIN;
                }
            }
        } 
        
    }while(1);
}

int DSPushTask::packet_ds_backup_request()
{
    int64_t pos = 0;
    M_Push.type = DS_BACKUP_CHUNK_PUSH;
    M_Push.request_id = RequestId;
    M_Push.length = sizeof(DsPush);
    M_Push.fileid = BlockInfo.fileid;
    M_Push.chunkid = BlockInfo.chunkid;
    M_Push.blocksize = BlockInfo.blocksize;
    if(M_Push.serialize(fPacketSend, kReqBufSize, pos) < 0)
        return -1;
    fPacketSendLen = pos;
    return 0;
}








