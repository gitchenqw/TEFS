//get stream from other dataserver
#include <sys/types.h>
#include "OS.h" 
#include "bserveripPref.h"
#include "DSPutTask.h"
#include "data_management.h"
#include "DSMessageProcess.h"
#include "DSServer.h"
#include <sys/syscall.h> 

#define __debug__ 0
#define gettid() syscall(__NR_gettid) 
extern tfs::dataserver::DataManagement* datamanager;
extern ServerPref* svrPref;

SInt64 DSPutTask::Run()
{
#if __debug__
    printf("dsputtask %d come in\n",this);
#endif
    EventFlags events = this->GetEvents();
    if (events & Task::kKillEvent)
        return -1;
     if (events & Task::kTimeoutEvent)
        return -1;
    if(fState == kInitial)
        fState = kRequestReceiving;
    OS_Error theErr = this->ProcessEvent(Task::kReadEvent);
    if((theErr == EINPROGRESS) || (theErr == EAGAIN))
    { 
        fSocket->RequestEvent(EPOLLIN|EPOLLET);
        return 0;
    }
    if(theErr != OS_NoErr)        //rollback
    {}
#if __debug__
    printf("dsputtask: %d exit,%s\n",this,strerror(theErr));
#endif
    return -1;
}

OS_Error DSPutTask::ProcessEvent(int /*eventBits*/)
{
    OS_Error theErr = OS_NoErr;
    if(!fSocket->IsConnected())
        return -1; 
    do
    {
        if(fState == kRequestProcessing)
        {
            ContentLen = M_Push.blocksize;
            RecvBlockdataOffset = 0;
            if(M_Push.fileid < 0|| M_Push.chunkid <0|| M_Push.blocksize < 0||ContentLen > blockinfo::MaxBlockSize)
            {
                LogError(FatalVerbosity,"Invalid argument,DS PUT request,blockid<%lld,%d>",
                                            M_Push.fileid,M_Push.chunkid);
                M_PushRes.retcode = PUTERROR;
                fState = kResponseSending;
            }
            else
            {   
                if(DataRecvLen >= ContentLen)
                {
                    if(datamanager->write_raw_data((uint64_t) M_Push.fileid,(uint32_t) M_Push.chunkid,
                                                    RecvBlockdataOffset, fBufferLen,fRecvBuffer) < 0)
                    {
                        LogError(FatalVerbosity,"write data error,when upload stream from server,blockid<%lld,%d>",
                                        M_Push.fileid,M_Push.chunkid);
                        M_PushRes.retcode = INTERNALERROR;
                    }
                    else
                    {
                        M_PushRes.retcode = PUTOK;
                        ADDRServer::GetServer()->IncrementRcvBytes(DataRecvLen);
                    }
                    fState = kResponseSending;
                }
                else
                    fState = kDataReceiving;
            }
        }
        if(fState == kDataReceiving)
        {
            UInt32 theRecvLen = 0;
            theErr = fSocket->Reads(&fRecvBuffer[fBufferLen], kDataBufSize - fBufferLen, &theRecvLen); 
            if (theErr == EAGAIN)
            {
#if __debug__
                LogError(DebugVerbosity,"No data when get stream from server,%d bytes has been received\n",DataRecvLen);
#endif          
                return theErr;
            }
            if(theErr != OS_NoErr)
            {
                LogError(WarningVerbosity,"Read data error,when get stream from server,blockid<%lld,%d>",
                                        M_Push.fileid,M_Push.chunkid);
                return theErr;
            }
            fBufferLen += theRecvLen;    //buffer
            DataRecvLen += theRecvLen;  //total receive
            ADDRServer::GetServer()->IncrementRcvBytes(theRecvLen);
            if(RecvBlockdataOffset + fBufferLen > blockinfo::MaxBlockSize)
            {
                LogError(WarningVerbosity,"Data range out of max block size,when get stream from server,blockid<%lld,%d>",
                                        M_Push.fileid,M_Push.chunkid);
                DataRecvLen = ContentLen;
            }
            if(fBufferLen == kDataBufSize || DataRecvLen >= ContentLen)
            {
                if(datamanager->write_raw_data((uint64_t) M_Push.fileid,(uint32_t) M_Push.chunkid,
                                                    RecvBlockdataOffset, fBufferLen,fRecvBuffer) < 0)
                {
                    LogError(FatalVerbosity,"Write data to disk error,when get stream from server,blockid<%lld,%d>",
                                        M_Push.fileid,M_Push.chunkid);
                    M_PushRes.retcode = INTERNALERROR;
                    fState = kResponseSending;
                }
                RecvBlockdataOffset += fBufferLen;  // add to offset
                fBufferLen = 0;                     // flush, buf available
            }
            if(DataRecvLen >= ContentLen)
            {    
                fState = kResponseSending;
                M_PushRes.retcode = PUTOK;
            }
        }
        if(fState == kResponseSending)
        {
            if(!HasGenResp)
            {
                if(M_PushRes.retcode == PUTOK)
                {
                    LogError(DebugVerbosity,"DS:%u get stream from server finished,blockid<%lld,%d>,length:%d",
                            svrPref->GetDataserverId(),M_Push.fileid,M_Push.chunkid,ContentLen);
                    MessageUtility::notify_ns_backup_ok(M_Push.fileid,M_Push.chunkid); 
                }
                packet_ds_push_resp();
                HasGenResp = true;
            }
            theErr = fSocket->Sendall(fPacketSend,fPacketSendLen);
            if(theErr == OS_NoErr)
                LogError(DebugVerbosity,"Send push response to server,length:%d,id:%d,type:0x%x,encrypt:%s,returncode:%d",
                      M_PushRes.length,M_PushRes.request_id,M_PushRes.type,(M_PushRes.encrypt ? "yes":"no"),M_PushRes.retcode);
            return theErr;
        } 
    }while(1);
}

int DSPutTask::packet_ds_push_resp()
{
    int64_t pos = 0;
    M_PushRes.type = DS_BACKUP_CHUNK_PUSH;
    M_PushRes.request_id = RequestId;
    M_PushRes.length = sizeof(DsPushRes);
    if(M_PushRes.retcode == 0) 
        return -1;
    if(M_PushRes.serialize(fPacketSend, kReqBufSize, pos) < 0)
        return -1;
    fPacketSendLen = pos;
    return 0;
}



