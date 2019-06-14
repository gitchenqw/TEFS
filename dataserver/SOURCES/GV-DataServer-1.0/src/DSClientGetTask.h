
#ifndef __DSClIENTGETTASK_H__
#define __DSClIENTGETTASK_H__

#include "Task.h"
#include "TimeoutTask.h"
#include "DSTCPServerSocket.h"
#include "DSTCPSession.h"
#include "ClientMessage.h"

class DSClientGetTask : public Task
{
public:
    enum
    {
        kMaxPacketSize = 8192,
        kReqBufSize = 1024,
        kDataBufSize = 1024
    };

   DSClientGetTask(ClientGet* CGet, TCPServerSocket* socket,SInt64 kIdleTimeoutInMsec = 10000):Task(),
    fTimeoutTask(this, kIdleTimeoutInMsec),
    M_Get(CGet),
    fSocket(socket),
    fState(kRequestProcessing),
    HasGenResp(false),
    fBufferLen(0),
    fPacketSendLen(0),
    real_read_len(0),
    busy(false),
    fMask(EPOLLIN|EPOLLET)
    {
        this->SetTaskName("GetTask");
        fSocket->SetTask(this);
        this->Signal(kReadEvent);
        fSocket->RequestEvent();
        this->SetThreadPicker(Task::GetBlockingTaskThreadPicker()); 
    }
    
    virtual ~DSClientGetTask() 
    {
        if(M_Get)
            free(M_Get);
        if(fSocket)
            delete fSocket;
    }
    virtual SInt64      Run();
    OS_Error             ProcessEvent(int /*eventBits*/) ;
    int                 packet_client_get_resp();
    
public:
    static UInt32      sActiveConnections;
    static UInt32      TotalTaskID;

private:
    TimeoutTask          fTimeoutTask;
    ClientGet*           M_Get;
    ClientGetRes         GetRes;
    TCPServerSocket*     fSocket;
    UInt32               TaskID;
    UInt32               fState;
    
    int                 RequestId;
    bool                HasGenResp;
    // request for receive  
    char                fRecvBuffer[kDataBufSize+1];
    UInt32              fBufferLen;
    //response for send
    char               fPacketSend[kMaxPacketSize];
    UInt32             fPacketSendLen;    
   
    // data send
    int32_t             read_offset;
    int32_t             real_read_len;
    bool                busy;
    int                 fMask;
};
#endif
