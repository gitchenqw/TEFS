
#include <sys/types.h>
#include "OS.h" 
#include "Message.h"
#include "DSMessageProcess.h"
#include "bserveripPref.h"
#include "DSClientGetTask.h"
#include "DSMessageProcess.h"
#include "DSServer.h"
#include "data_management.h"
#include <sys/syscall.h> 

#define __debug__ 0
#define gettid() syscall(__NR_gettid) 

UInt32       DSClientGetTask::TotalTaskID = 0;
UInt32       DSClientGetTask::sActiveConnections = 0;
extern tfs::dataserver::DataManagement* datamanager;

SInt64 DSClientGetTask::Run()
{
#if __debug__
    printf("clientgettask: %d come in\n",TaskID);
#endif
    EventFlags events = this->GetEvents();
    if (events & Task::kKillEvent)
        return -1;
     if (events & Task::kTimeoutEvent)
     {
        LogError(WarningVerbosity,"Send data to client timeout,%d bytes sended",read_offset);
        return -1;
     }
    fTimeoutTask.RefreshTimeout();
    if(fState == kInitial)
        fState = kRequestReceiving;
    OS_Error theErr = this->ProcessEvent(Task::kReadEvent);
    if((theErr == EINPROGRESS) || (theErr == EAGAIN))
    { 
        fSocket->RequestEvent(fMask);
        return 0;
    }
#if __debug__
    printf("clientputtask: %d exit,%s\n",TaskID,strerror(theErr));
#endif
    LogError(DebugVerbosity,"Send data to client,%d bytes sended",read_offset);
    return -1;
}

OS_Error DSClientGetTask::ProcessEvent(int /*eventBits*/)
{
    OS_Error theErr = OS_NoErr;
    if(!fSocket->IsConnected())
        return -1; 
    do
    {
        if(fState == kRequestProcessing)
        {
            GetRes.retcode = GETOK;
            if(M_Get->start<0||M_Get->end < M_Get->start||blockinfo::MaxBlockSize < M_Get->end - M_Get->start+1)
            {
                LogError(FatalVerbosity,"Invalid argument,when client GET,blockid<%lld,%d>",
                                            M_Get->fileid,M_Get->chunkid);
                GetRes.retcode = GETERROR;
            }
            int32_t blocksize,visitcount;
            if(datamanager->get_block_info(M_Get->fileid,M_Get->chunkid,blocksize,visitcount) < 0)
                GetRes.retcode = NOTFOUND;
            fState = kResponseSending;
            fMask = EPOLLOUT|EPOLLET;
        }
        if(fState == kResponseSending)
        {
            if(!HasGenResp)
            {
                if( packet_client_get_resp() < 0)
                    LogError(DebugVerbosity,"packet client get resp error\n");
                HasGenResp = true;
            }
            LogError(DebugVerbosity,"Send response to client,length:%d,id:%d,type:0x%x,encrypt:%s,returncode:%d",
                      GetRes.length,GetRes.request_id,GetRes.type,(GetRes.encrypt ? "yes":"no"),GetRes.retcode);
            theErr = fSocket->Sendall(fPacketSend,fPacketSendLen);
            if(theErr != OS_NoErr)
            {
                return theErr;
            }
            if(GetRes.retcode == GETOK)
            {
                read_offset = M_Get->start;
                fState = kDataSending;
            }
            else
            {
                return theErr;
            }
        } 
        
        if(fState == kDataSending)
        {
            do{
                if(!busy)
                {
                    real_read_len = kMaxPacketSize;
                    if(read_offset + real_read_len > M_Get->end)
                        real_read_len = M_Get->end - read_offset+1;
                    if(datamanager->read_raw_data(M_Get->fileid,M_Get->chunkid,read_offset,
                                                real_read_len,fPacketSend) < 0)
                    {
                        LogError(DebugVerbosity,"read disk data error,length:%d,id:%d,type:0x%x,encrypt:%s,returncode:%d",
                            GetRes.length,GetRes.request_id,GetRes.type,(GetRes.encrypt ? "yes":"no"),GetRes.retcode);
                        return theErr;
                    }
                    read_offset += real_read_len;
                }
                theErr = fSocket->Sendall(fPacketSend,real_read_len); 
                if(theErr != OS_NoErr)
                {
                    busy = true;
                    return theErr;
                }
                //statistic sent bytes
                ADDRServer::GetServer()->IncrementSndBytes(real_read_len);
                busy = false;
            }while(read_offset <= M_Get->end);
            fMask = EPOLLIN|EPOLLET;
            fState = kTasksessionEnd;
            return EAGAIN;
        }
        if(fState == kTasksessionEnd)
            return -1;
    }while(1);
}

int DSClientGetTask::packet_client_get_resp()
{
    int64_t pos = 0;
    GetRes.type = CLIENT_GET;
    GetRes.request_id = M_Get->request_id;
    GetRes.length = sizeof(ClientGetRes);
    if(GetRes.retcode == 0) 
        return -1;
    if(GetRes.serialize(fPacketSend, kReqBufSize, pos) < 0)
        return -1;
    fPacketSendLen = pos;
    return 0;
}




