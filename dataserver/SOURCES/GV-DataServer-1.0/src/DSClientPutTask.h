
#ifndef __DSClIENTPUTTASK_H__
#define __DSClIENTPUTTASK_H__

#include "DSTCPServerSocket.h"
#include "DSTCPSession.h"
#include "Task.h"
#include "ClientMessage.h"

class DSClientPutTask : public Task
{
public:
    enum
    {
        kMaxPacketSize = 1024,
        kReqBufSize = 1024,
        kDataBufSize = 1024
    };

    DSClientPutTask(ClientPut* CPut, TCPServerSocket* socket, char* data, int datalen);

    virtual             ~DSClientPutTask();
    virtual SInt64      Run();
    OS_Error             ProcessEvent(int /*eventBits*/) ;
    int                 packet_client_put_resp();
    
public:
    static UInt32      sActiveConnections;
    static UInt32      TotalTaskID;

private:
    ClientPut*           M_Put;
    ClientPutRes         PutRes;
    TCPServerSocket*     fSocket;
    UInt32               TaskID;
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
