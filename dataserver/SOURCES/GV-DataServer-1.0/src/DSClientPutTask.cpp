
#include <sys/types.h>
#include "OS.h" 
#include "Message.h"
#include "bserveripPref.h"
#include "DSClientPutTask.h"
#include "DSPushTask.h"
#include "data_management.h"
#include "DSMessageProcess.h"
#include "DSServer.h"
#include <sys/syscall.h> 

#define __debug__ 0
#define gettid() syscall(__NR_gettid) 

UInt32       DSClientPutTask::TotalTaskID = 0;
UInt32       DSClientPutTask::sActiveConnections = 0;
extern tfs::dataserver::DataManagement* datamanager;
extern ServerPref* svrPref;

DSClientPutTask::DSClientPutTask(ClientPut* CPut, TCPServerSocket* socket, char* data, int datalen):Task(),
    M_Put(CPut),
    fSocket(socket),
    fState(kRequestProcessing),
    HasGenResp(false),
    fPacketSendLen(0)
    {
        this->SetTaskName("ClientPutTask");
        if(datalen > 0 && datalen < kDataBufSize)
            ::memcpy(fRecvBuffer, data,datalen);
         DataRecvLen = datalen;
        fBufferLen  = datalen;
        fSocket->SetTask(this);
        this->Signal(kReadEvent);
        fSocket->RequestEvent();
        this->SetThreadPicker(Task::GetBlockingTaskThreadPicker()); 
    }
    DSClientPutTask::~DSClientPutTask()
    {
        if(M_Put)
            free(M_Put);
        if(fSocket)
            delete fSocket;
    }

SInt64 DSClientPutTask::Run()
{
#if __debug__
    printf("clientputtask: %d come in\n",TaskID);
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
        fSocket->RequestEvent();
        return 0;
    }
    //push stream to other dataserver,when the block is complete
    if(theErr == OS_NoErr)   
    {
        for(int i=0;i< M_Put->backup_dataservernum;i++)
        {
            new DSPushTask(M_Put->address[i].ip,M_Put->address[i].port,M_Put->fileid,M_Put->chunkid,M_Put->start,M_Put->end);
        }
    }
    if(theErr != OS_NoErr)        //rollback
    {}
#if __debug__
    printf("clientputtask: %d exit,%s\n",TaskID,strerror(theErr));
#endif
    return -1;
}

OS_Error DSClientPutTask::ProcessEvent(int /*eventBits*/)
{
    OS_Error theErr = OS_NoErr;
    if(!fSocket->IsConnected())
        return -1; 
    do
    {
        if(fState == kRequestProcessing)
        {
            ContentLen = M_Put->end - M_Put->start+1;
            RecvBlockdataOffset = M_Put->start;
            if(M_Put->fileid < 0|| M_Put->chunkid <0||M_Put->start< 0||M_Put->end < M_Put->start||
                ContentLen > blockinfo::MaxBlockSize||RecvBlockdataOffset > blockinfo::MaxBlockSize)
            {
                LogError(FatalVerbosity,"Invalid argument,when client PUT,blockid<%lld,%d>",
                                            M_Put->fileid,M_Put->chunkid);
                PutRes.retcode = PUTERROR;
                fState = kResponseSending;
            }
            else
            {   
                if(DataRecvLen >= ContentLen)
                {
                    if(datamanager->write_raw_data((uint64_t) M_Put->fileid,(uint32_t) M_Put->chunkid,
                                                    RecvBlockdataOffset, fBufferLen,fRecvBuffer) < 0)
                    {
                        LogError(FatalVerbosity,"write data error,when upload stream from client,blockid<%lld,%d>",
                                        M_Put->fileid,M_Put->chunkid);
                        PutRes.retcode = INTERNALERROR;
                    }
                    else
                    {
                        PutRes.retcode = PUTOK;
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
                return theErr;
            }
            if(theErr != OS_NoErr)
            {
                LogError(WarningVerbosity,"Read data from client error,content length:%d,total %d bytes is recived,blockid<%lld,%d>",
                         ContentLen,DataRecvLen,M_Put->fileid,M_Put->chunkid);
                return theErr;
            }  
            ADDRServer::GetServer()->IncrementRcvBytes(theRecvLen);
            fBufferLen += theRecvLen;    //buffer
            DataRecvLen += theRecvLen;  //total receive
            if(RecvBlockdataOffset + fBufferLen > blockinfo::MaxBlockSize)
            {
                LogError(WarningVerbosity,"Data range out of max block size,when data receiving from client,blockid<%lld,%d>",
                                        M_Put->fileid,M_Put->chunkid);
                DataRecvLen = ContentLen;
            }
            if(fBufferLen == kDataBufSize || DataRecvLen >= ContentLen)
            {
                if(datamanager->write_raw_data((uint64_t) M_Put->fileid,(uint32_t) M_Put->chunkid,
                                                    RecvBlockdataOffset, fBufferLen,fRecvBuffer) < 0)
                {
                    LogError(FatalVerbosity,"write data error,when upload stream from client,blockid<%lld,%d>",
                                        M_Put->fileid,M_Put->chunkid);
                    PutRes.retcode = INTERNALERROR;
                    fState = kResponseSending;
                }
                RecvBlockdataOffset += fBufferLen;  // add to offset
                fBufferLen = 0;                     // flush, buf available
            }
            if(DataRecvLen >= ContentLen)
            {    
                fState = kResponseSending;
                PutRes.retcode = PUTOK;
            }
        }
        if(fState == kResponseSending)
        {
            if(!HasGenResp)
            {
                if(PutRes.retcode == PUTOK)
                {
                LogError(DebugVerbosity,"Server:%u upload stream from client finished,"
                         "blockid<%lld,%d>,range: %d-%d,content length:%d,receive data length:%d,"
                         "notify nameserver",svrPref->GetDataserverId(),M_Put->fileid,
                          M_Put->chunkid,M_Put->start,M_Put->end,ContentLen,DataRecvLen);
                MessageUtility::notify_ns_backup_ok(M_Put->fileid,M_Put->chunkid); 
                }
                packet_client_put_resp();
                HasGenResp = true;
            }
            LogError(DebugVerbosity,"Send response to client,length:%d,id:%d,type:0x%x,encrypt:%s,returncode:%d",
                      PutRes.length,PutRes.request_id,PutRes.type,(PutRes.encrypt ? "yes":"no"),PutRes.retcode);
            
            theErr = fSocket->Sendall(fPacketSend,fPacketSendLen);
            return theErr;
        } 
    }while(1);
}

int DSClientPutTask::packet_client_put_resp()
{
    int64_t pos = 0;
    PutRes.type = CLIENT_PUT;
    PutRes.request_id = M_Put->request_id;
    PutRes.length = sizeof(ClientPutRes);
    if(PutRes.retcode == 0) 
        return -1;
    if(PutRes.serialize(fPacketSend, kReqBufSize, pos) < 0)
        return -1;
    fPacketSendLen = pos;
    return 0;
}



