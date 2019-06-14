
#ifndef __DSPUSHTASK_H__
#define __DSPUSHTASK_H__

#include "DSTCPServerSocket.h"
#include "DSTCPSession.h"
#include "Task.h"
#include "DsMessage.h"

class DSPushTask : public Task
{
public:
    enum
    {
        kMaxPacketSize = 1024,
        kReqBufSize = 1024,
        kDataBufSize = 1024
    };
    DSPushTask(char ip[16],int port,SInt64 fileid,SInt32 chunkid,SInt32 start,SInt32 end);
    virtual ~DSPushTask() 
    {
    }
    virtual SInt64      Run();
    OS_Error             ProcessEvent(int /*eventBits*/) ;
    int                 packet_ds_backup_request();
    
public:
    static UInt32      TotalTaskID;

private:
    DsPush               M_Push;
    DsPushRes            M_PushRes;
    blockinfo            BlockInfo;
    TCPServerSocket      fSocket;
#if IPV6
   `char                RemoteIp[INET_ADDRSTRLEN];
    char                RemoteIpStr[INET6_ADDRSTRLEN];
#else
    UInt32               RemoteIp;
    char                RemoteIpStr[INET_ADDRSTRLEN];
#endif
    
    UInt16               RemotePort;
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
    
    //block data send
    SInt32             read_offset;
    SInt32             real_read_len;
    SInt32             read_start;
    SInt32             read_end;
    UInt32             NeedSend;
    bool                busy;
    int                 fMask;
};
#endif
