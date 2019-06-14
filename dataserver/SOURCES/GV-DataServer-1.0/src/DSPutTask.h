
#ifndef __DSPUTTASK_H__
#define __DSPUTTASK_H__

#include "DSTCPServerSocket.h"
#include "DSTCPSession.h"
#include "Task.h"
#include "DsMessage.h"

class DSPutTask : public Task
{
public:
    enum
    {
        kMaxPacketSize = 1024,
        kReqBufSize = 1024,
        kDataBufSize = 1024
    };

   DSPutTask(DsPush CPut, TCPServerSocket* socket, char* data, int datalen):Task(),
    M_Push(CPut),
    fSocket(socket),
    fState(kRequestProcessing),
    HasGenResp(false),
    fPacketSendLen(0)
    {
        this->SetTaskName("DsPutTask");
        if(datalen > 0 && datalen < kDataBufSize)
            ::memcpy(fRecvBuffer, data,datalen);
         DataRecvLen = datalen;
        fBufferLen  = datalen;
        fSocket->SetTask(this);
        this->Signal(kReadEvent);
        fSocket->RequestEvent();
        this->SetThreadPicker(Task::GetBlockingTaskThreadPicker()); 
    }
    
    virtual ~DSPutTask() 
    {
        if(fSocket)
            delete fSocket;
    }
    virtual SInt64      Run();
    OS_Error             ProcessEvent(int /*eventBits*/) ;
    int                 packet_ds_push_resp();

private:
    DsPush               M_Push;
    DsPushRes            M_PushRes;
    TCPServerSocket*     fSocket;
    UInt32               fState;
    
    int                 RequestId;
    bool                HasGenResp;
    // request for receive  
    char                fRecvBuffer[kDataBufSize+1];
    UInt32              fBufferLen;
    // data receive
    SInt32               RecvBlockdataOffset;
    SInt32               ContentLen;
    SInt32               DataRecvLen;
    //response for send
    char               fPacketSend[kMaxPacketSize];
    UInt32             fPacketSendLen;
};
#endif
