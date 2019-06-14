
#ifndef __DSTCPSESSION_H__
#define __DSTCPSESSION_H__

#include "DSTCPServerSocket.h"
#include "Task.h"
#include "DSErrorLog.h"
enum 
{ 
    kInitial,
    kRequestReceiving, 
    kRequestProcessing,
    kRequestReceived,
    kDataReceiving,
    kRequestSending,
    kDataSending,
    kResponseSending,
    kResponseReceiving,
    kTasksessionEnd
};

class DSTCPSession : public Task
{
public:
    enum
    {
        kMaxPacketSize = 1024,
        kReqBufSize = 1024,
        kDataBufSize = 1024
    };
    DSTCPSession():     
    Task(),
    fState(kInitial),
    fRecvLen(0),
    fRequestLen(0)
    {
        fSocket = new TCPServerSocket(Socket::kNonBlockingSocketType);
        this->SetTaskName("TCPSession");
        ::memset(fRecvBuffer, 0,kReqBufSize + 1);
        TaskID = TotalTaskID++;
    }
    
    ~DSTCPSession() 
    {
    }
    virtual SInt64      Run();
    TCPServerSocket*     GetSocket()         { return  fSocket;}
    OS_Error             ProcessEvent(int /*eventBits*/) ;
public:
    static UInt32      sActiveConnections;
    static UInt32      TotalTaskID;

private:
    TCPServerSocket*     fSocket;
    UInt32               TaskID;
    UInt32               fState;
    int                  MessageType;
    
    // request for receive  
    char                fRecvBuffer[kReqBufSize+1];
    UInt32              fRecvLen;
    UInt32              fRequestLen;
};
#endif
